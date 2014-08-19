# Demo of my skills
Demo consisting of few files from a school project - interpreter of a language
IFJ12 (subset of Falcon) written in C. I was a team leader (4 members of team including me) and I implemented and designed a significant part of this project.
Sorry for the comments in Czech, but it was easier for the team to use Czech.

### You can see 5 modules I created.

* **ilist**, **label_stack** and **runtime_stack** modules are implementations of data structures, since we weren't allowed to use libraries for these kinds of things.

* **parser_dropdown** module implements a dropdown parser for syntax analysis of the source file, using other modules created by the rest of the team to create a list of instructions that will be interpreted by interpreter

* **interpreter** implements the final part, which takes a list of intstructions and executes them.
