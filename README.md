# BuildTool For C
Can compile C code by recursively descending into directories and launching processes which compile a file of code by calling GCC. Each directory (except the topmost) will make a .a (code archive or static library). The topmost directory will create an executable.
