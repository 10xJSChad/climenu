## climenu
C application for creating interactive shell menus with custom buttons. Create menus from a text file, navigate using keys, and execute shell commands easily. Useful for simplifying command execution and organization.


https://github.com/10xJSChad/climenu/assets/48174610/3fdb2471-ec67-4a28-b369-415ae78f04d5


```
Usage:
  climenu [ENTRIES FILE]
  An example of what an entries file should look like can be found in 'example.conf'

Entry format:
  -Labels-
  [Header]: Defines a menu header with a text description.
  [Entry]: Defines a button with a label and a shell command to execute.
  
  -Properties-
  Header and Entry:
  str: Specifies the text to display for the header or button.
  
  Entry:
  exec: Specifies the shell command to execute when the button is pressed.
  wait: Optional. If set to true, the program waits for a key press after executing the command.

```

```
Requirements:
  Linux (Other *nix-es probably work fine, too)
  A C compiler.

Building:
  cc climenu.c
```
