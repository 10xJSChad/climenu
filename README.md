## climenu
C application for creating interactive text menus with custom buttons. Menus can be read from config files or stdin, navigate using keys, and execute shell commands easily. Useful for simplifying command execution and organization, but can also be used to quickly create more advanced menu applications.<br><br>
Do not question why it's called **cli**menu when it creates tuis.

If you're looking for something even simpler, I also made [cmenu](https://github.com/10xJSChad/cmenu), which is pretty much just a terminal dmenu clone. (cmenu does not have Windows support.)

### Basic example (example.conf):

https://github.com/10xJSChad/climenu/assets/48174610/e9f5be09-ea46-42a6-803d-6b3e71315952

### climenu in a file explorer (explorer.sh):

https://github.com/10xJSChad/climenu/assets/48174610/f4361d28-c396-419e-8892-9184aa2852e9


<br>

### Features:
  - Incredibly simple config syntax, creating entries is a breeze.
  - Can read entries from stdin, allowing for dynamic generation of menus by external programs.
  - No external dependencies, it'll easily compile on anything that resembles Linux.
  - Runs just fine on any terminal emulator that supports common ANSI escape codes.

<br>

```
Usage:
 climenu [ENTRIES PATH] [UPDATE INTERVAL]
 If no entries file is provided, climenu
 will attempt to read entries from stdin.

Keys:
  Navigate: JK, UP/DOWN arrows.
  Select:   Space, Enter
  Exit:     CTRL+C, Q

Entry types:
 [Header]: Defines a menu header with a text description.
 [Entry]:  Defines a button with a label and a shell command to execute.
 
Entry properties:
 [Header] and [Entry]:
   <str>:   Specifies the text to display for the header or button.
   fgcolor: foreground color. [black,blue,cyan,green,magenta,red,white,yellow]
   bgcolor: background color. [black,blue,cyan,green,magenta,red,white,yellow]
   runstr:  Execute the entry string and display the result [true]

 [Entry]:
   <exec>:    Specifies the shell command to execute when the button is pressed.
   exit:      Exit after executing an entry command. [true]
   wait:      Waits for a key press after executing the command. [true]
   colormode: When the entry should be displayed in color. [selected]
```

### Contribution Guidelines:
  * Do not refactor unless you're already adding or fixing something. If the code has nothing to do with what you're trying to accomplish, don't refactor it. Do feel free to change anything in the vicinity of the code you *are* working on, though.
  * Keep the scope of your contribution small and simple, this application is not supposed to be feature-packed and super customizable. Try to keep your features as general as possible, climenu doesn't need to meet the needs of every user.
  * The program must still compile with a simple ```gcc climenu.c``` when you're done, this is less of a guideline and more of a hard rule. (Also please don't break the other platform's compilation lol.)

<br>


```
Requirements:
  Linux (Other *nix-es probably work fine, too)
  A C compiler.

Building:
  cc climenu.c
```

### ...alternatively, if you want to use the experimental Windows version.

```
Requirements:
  Windows

Building:
  Honestly I just compiled it in VS2022, I don't know the command for this, definitely MSVC though.
```

A Windows release is provided, *nix users will have to compile it themselves.
The Windows and *nix versions of ```climenu``` will behave differently at times, most notably when piping *to* ```climenu```.
I'd like them to behave identically, but I'm not very familiar with Windows TUI stuff, so that'll have to wait.
For now though, in most circumstances it behaves just as expected.
  

