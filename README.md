## Description

Reads and displays a custom menu from a JSON file. Menu entries specify a command, which can be called with or without arguments.

![Screenshot](https://marvinkreis.github.io/rofi-plugins/rofi-prompt/example.png)

### Menu File Format

Menu entries are read from a JSON file (default: `XDG_CONFIG_HOME/rofi-json-menu`) with the following format:

```json
{
    "firefox": { },
    "foo": {
        "cmd":          "some-script",
        "icon":         "terminal"
    },
    "mail": {
        "description":  "thunderbird",
        "cmd":          "thunderbird",
        "terminal":     false,
        "icon":         "mail"
    }
}
```

All properties are optional with default values:

Property      | Default Value | Description
------------- | ------------- | -----------
`description` | none          | description to display next to the name
`cmd`         | entry name    | command to execute
`terminal`    | false         | whether to open the command in a terminal or not
`icon`        | entry name    | name of the icon to display

### Matching

This plugin uses custom matching to make command line arguments possible:

Input                         | Executes ...                   | Example
----------------------------- | ------------------------------ | -------
prefix of an entry            | entry's command                | `fo`
complete entry with arguments | entry's command with arguments | `foo arg`
custom input                  | custom input                   | `loffice`

### Command line options / Configuration

Option                               | Description
------------------------------------ | -----------
`-json-menu-file <path>`             | Set the menu file (default: `XDG_CONFIG_HOME/rofi-json-menu`).
`-json-menu-disable-icons`           | Disable icons (default: enabled).
`-json-menu-icon-theme <theme name>` | Set the icon theme (default: `Adwaita`).

Theme options can be used multiple times to set fallback themes.
The source file contains more configuration via `#define`.

## Compilation

### Dependencies

Dependency | Version
---------- | -------
rofi       | 1.4
json-c     | 0.13

### Installation

Use the following steps to compile the plugin with the **autotools** build system:

```bash
git submodule init
git submodule update

autoreconf -i
./configure
make
make install
```
