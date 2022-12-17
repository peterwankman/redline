Introduction
============

Edward is an editor, inspired by EDLIN, which was included in MS-DOS, but
unceremoniously dumped by Microsoft in later operating systems.

This program is not, however an exact reimplementation, which is why I'm
going to describe here what it actually does, command by command.

The biggest change is, that all strings need to be quoted. Where EDLIN
would let you search for a string by Sstring, in Ede, you would write
S"string", and so on for any command that takes strings.

If you are required to enter multiple lines, you end the input by typing a
single line with only a single period.

The editor keeps track of a cursor, pointing to the last line that was
accessed in some fashion. The cursor will be indicated by an asterisk when
that particular line is printed, for instance when using the L command.
In any command which refers to one or multiple lines to be used, you can refer
to the line with the cursor by using a period as the line to use. For example
"1,.L" will list the file from beginning until the cursor.

COMMAND LINE:
=============

* Usage: [binary] [-b] [-h] [-p prompt] [-v] filename

-b: Ignore EOL/EOF characters.
-h: Print the command line options (like described here).
-p: Change the prompt from the default "*". 
-v: Print version and licensing information.

The filename argument is not optional. If the file doesn't exist, it will ne
created when ending the session or explicitely saving.

COMMANDS:
=========

?:Manual
----------
* Usage: ```?```

Gives a brief description of all commands.

[line]: Edit line.
------------------
* Usage: ```[line]```

Prompts you to enter a single line, which will replace the one given.

A: Append
---------
* Usage: ```[lines]A```

This command is a complete mystery to me. I can't make EDLIN do anything
other than reply "End of input file", whatever I try to feed it. I took
that as an invitation to be creative and I found it obvious to let you
append text at the end of the file. Using the optional argument will let
you enter that number of lines before going back to the prompt. Without
any argument, end the input as described above.

C: Copy
-------
* Usage: ```[start],[end],target[,repetitions]C```

Copy a line or block of text to another place in the file, optionally
repeating it multiple times. The first line of the copied text will end up
at the target line.

D: Delete
---------
* Usage: ```[start][,end]D```

Like it says. Delete a line or block of text.

E:End
-------
* Usage: ```E```

Saves the file and exits.

I:Insert
----------
* Usage: ```[line]I```

Lets you enter a block of text, the first line entered being inserted in
place of the number given as the argument.

L:List
--------
* Usage: ```[start][,end]L```

Lists the file from start to end. When given no argument, it will begin
listing 11 lines before the cursor. After a full page of 24 lines has been
printed, it will ask you if you want to continue.

M:Move
--------
* Usage: ```[start],[end],target```

Cuts a block of text and pastes it at the target line. The first line of the
block will end up on the target line.

P:Page
--------
* Usage: ```[start][,end]P```

Listing behaves like L, but will start at the cursor instead of 11 lines
before. P will set the cursor to the last displayed line. Running P again
or on its own will continue from the line after the cursor.

Q:Quit
--------
* Usage: ```Q```

Exit without saving changes.

R:Search and replace
----------------------
* Usage: ```[start][,end][?]R[search],[replace]```

Replaces all instances of the search string within the range of lines with
the given replacement string. With the question mark in the command, it
will display the change that will be made and prompt you to confirm before
actually making the change.

S:Search
----------
* Usage: ```[start][,end][?]S[search]```

Puts the cursor at line with the first occurrence of the search string.
With the question mark in the command, it will prompt you whether to
end the search or to continue.
After an initial search, the command can be used again without giving a
new search string, to continue the last search operation. The previous
range to search in will be discarded for the new one (that means, if you
don't give a range, it will search from the cursor to the end of the
file.)

T:Transfer
------------
* Usage: ```[target]T[filename]```

This will load the given file and place its contents in the document at
the given line number. The first line of the file will be pasted to the
given target line.

W:Write
--------- 
* Usage: ```[lines]W[filename]```

Saves the file from the beginning of the buffer to the given line. Without
argument, the whole buffer will be written to disk. If a filename is given,
the buffer will only be saved to that file, not the originally opened file.
