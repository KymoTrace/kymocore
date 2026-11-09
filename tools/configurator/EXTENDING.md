# Extending the configurator

Both GUI modes are generated from one Qt-free catalog
(`src/kymo_configurator/catalog.py`). A new KymoCore feature therefore needs no
changes to the windows: describe it once and it appears as a page in the
professional mode and as a step in the guided mode.

## Building blocks

| Hook | Module | Purpose |
|---|---|---|
| `register_section(Section)` | `catalog` | New page/step with explained fields |
| `register_widget(name, factory)` | `gui.widgets` | Custom editor (tables, previews) used by `Section.widgets` |
| `register_validator(fn, top_level_fields)` | `validation` | Checks plus the project fields the extension owns |
| `register_renderer(fn)` | `generation` | Extra generated files in every snapshot |

A `Field` carries everything the UI shows: label, plain-language help, the
affected C macro or signature (`target`), the editor kind, limits, whether it is
`advanced` (hidden in guided mode), when it is enabled (`enabled_when` +
`requires` explains why not) and an optional `on_change` hook. Diagnostics are
routed to a section through `diagnostic_prefixes`, so problems show up at the
right field, in the navigation status and in the guided step.

## Example: a custom command extension

This is an illustrative extension, not the built-in parameter/watch back channel.
`KYMO_ENABLE_BACKCHANNEL` is a placeholder defined by the example, not a Core
setting. Existing parameters and live variables use `KYMO_ENABLE_PARAMETERS`
and `KYMO_ENABLE_WATCH`.

```python
from kymo_configurator import catalog
from kymo_configurator.generation import register_renderer
from kymo_configurator.validation import Diagnostic, register_validator

def _on(view):
    return view.enabled

catalog.register_section(catalog.Section(
    "backchannel", "Back channel",
    "Lets KymoStudio send commands and parameter changes to the device.",
    groups=(catalog.Group("Reception", (
        catalog.Field("backchannel.enabled", ("backchannel", "enabled"),
                      "Receive commands", "Compiles the command receiver.",
                      target="KYMO_ENABLE_BACKCHANNEL", enabled_when=_on,
                      requires="Enable KymoCore for this profile first."),
        catalog.Field("backchannel.read", ("backchannel", "read"), "Read bytes",
                      "Returns received bytes from your driver.", kind="identifier",
                      default="", target="size_t read(void *user, uint8_t *out, size_t max)"),
    )),),
    guided_title="Should the device accept commands?",
    guided_intro="KymoStudio can change parameters at run time.",
    diagnostic_prefixes=("backchannel",),
    summary=lambda view: "on" if view.get(("backchannel", "enabled"), False) else "off",
    order=45))

def validate_backchannel(project, profile):
    data = project.get("backchannel", {})
    if data.get("enabled") and not data.get("read"):
        return [Diagnostic("backchannel.read", "error", "backchannel.read",
                           "A read function is required")]
    return []

register_validator(validate_backchannel, top_level_fields=("backchannel",))
register_renderer(lambda project, profile, resolved: {
    "kymo_backchannel.h": "#define KYMO_ENABLE_BACKCHANNEL %d\n"
    % int(bool(project.get("backchannel", {}).get("enabled")))})
```

Register extensions before the GUI or CLI runs (for example in the module that
adds the feature to the package). When a feature becomes part of KymoCore, move
its section into `catalog.SECTIONS`, its checks into `validation.py` and bump the
project schema with a migration step in `migration.py`.

`tests/test_catalog.py::ExtensionTests` and
`tests/test_gui_workflows.py::test_extension_section_appears_in_both_modes`
show the complete round trip.
