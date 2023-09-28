## climenu
C application for creating interactive shell menus with custom buttons. Create menus from a text file, navigate using keys, and execute shell commands easily. Useful for simplifying command execution and organization.


https://github.com/10xJSChad/climenu/assets/48174610/3fdb2471-ec67-4a28-b369-415ae78f04d5

<br>

### Features:
  - Incredibly simple config syntax, adding entries is a breeze.
  - No external dependencies, it'll easily compile on anything that resembles Linux.
  - Runs just fine on any terminal emulator that supports common ANSI escape codes.

<br>

```
Usage:
  climenu [ENTRIES FILE]
  An example of what an entries file should look like can be found in 'example.conf'

Entry format:
  -Labels-
  [Header]: Defines a menu header with a text description.
  [Entry]: Defines a button with a label and a shell command to execute.
  
  -Properties-
  All:
    str:     Specifies the text to display for the header or button.
    fgcolor: (Optional) foreground color. [Values: black, blue, cyan, green, magenta, red, white, yellow]
    bgcolor: (Optional) background color. [Values: black, blue, cyan, green, magenta, red, white, yellow]

  Entry:
    exec:      Specifies the shell command to execute when the button is pressed.
    wait:      (Optional) If set to true, the program waits for a key press after executing the command.
    colormode: (Optional) When the entry should be displayed in color. [Values: selected]
```


### Roadmap:
  - [ ]  Scrolling, currently it does not scroll and thus you can't *really* have a whole lot of entries.
  - [ ]  More dynamic entry text, would be nice if an entry could conditionally change its text and color.
  - [ ]  Not a feature, but the parser isn't great to look at, should be rewritten for clarity.


### Contribution Guidelines:
  * Do not refactor unless you're already adding or fixing something. If the code has nothing to do with what you're trying to accomplish, don't refactor it. Do feel free to change anything in the vicinity of the code you *are* working on, though.
  * Keep the scope of your contribution small and simple, this application is not supposed to be feature-packed and super customizable. Try to keep your features as general as possible, climenu doesn't need to meet the needs of every user.
  * The program must still compile with a simple ```gcc climenu.c``` when you're done, this is less of a guideline and more of a hard rule.

<br>


```
Requirements:
  Linux (Other *nix-es probably work fine, too)
  A C compiler.

Building:
  cc climenu.c
```
