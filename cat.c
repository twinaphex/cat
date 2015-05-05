/*
	cat implementation for milestone 3.

	Author: Lee Fallat (5004785)
*/

#include <unistd.h>
#include <getopt.h> // Needs to be included if -std=c99...
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

/*
	Constants
*/
#define TAB 0x09
#define PRINTABLE_CHARACTERS_START 0x20
#define ALPHABET_START 0x41-1

/*
	A data structure to hold all of our options.
*/
typedef struct {
	int line_numbers;
	int end_of_line;
	int show_non_printing;
	int show_tabs;
	int line_number_no_blank;
	int suppress_many_blank_lines;
	unsigned int files;
	unsigned int files_start;
	int no_options;
} CatOpts;


/* A data structure to hold all of our program's state. */
typedef struct {
	int input;
	int current_line;
	int beginning_of_line;
	int last_char;
	int modified_char;
	int suppress_output;
	int multiple_newlines;
	int end_of_line;
} CatState;

/* Creates a new initialized CatState data structure. */
CatState * new_CatState() {
	CatState *state = malloc(sizeof(CatState));
	state->input = 0x00;
	state->current_line = 1;
	state->beginning_of_line = 1;
	state->last_char = 0x00;
	state->modified_char = 0;
	state->suppress_output = 0;
	state->multiple_newlines = 0;
	state->end_of_line = 0;

	return state;
}


/* Creates a new initialized CatOpts data structure. */
CatOpts * new_CatOpts() {
	CatOpts *options = malloc(sizeof(CatOpts));
	options->line_numbers = 0;
	options->end_of_line = 0;
	options->show_non_printing = 0;
	options->show_tabs = 0;
	options->line_number_no_blank = 0;
	options->suppress_many_blank_lines = 0;
	options->files_start = 0;
	options->files = 0;
	options->no_options = 0;
	
	return options;
}

/*
	Parses all of the users invoked options, and stores them in a CatOpts
	data structure to be used later.
*/
CatOpts * parse_cat_options(int noptions, char *argv[]) {
	CatOpts *options = new_CatOpts();
	int option_identifier;
	
	while ((option_identifier = getopt(noptions, argv, "nEbstve")) != -1) {
		switch(option_identifier) {
			case 'n':
				options->line_numbers = 1;
				break;
			case 'E':
				options->end_of_line = 1;
				break;
			case 'b':
				options->line_number_no_blank = 1;
				options->line_numbers = 1;
				break;
			case 's':
				options->suppress_many_blank_lines = 1;
				break;
			case 't':
				options->show_tabs = 1;
			case 'v':
				options->show_non_printing = 1;
				break;
			case 'e':
				options->show_non_printing = 1;
				options->end_of_line = 1;
				break;
			case '?':
				fprintf(stderr, "You have provided an unknown option -%c.\n", optopt);
			default:
				break;
		}
	}
	
	options->files_start = optind;
	options->files = noptions - optind;
	if(optind == 0)
		options->no_options = 1;
	
	return options;
}

/*
	If we want to suppress multiple blank lines, check to see if
	we have detected multiple lines. If so, we will suppress them.
*/
void suppress_output(CatState *state) {
	if(state->multiple_newlines) {
			state->suppress_output = 1;
	}
}

/*
	If this option is on, print line numbers.
	If -n, print line numbers on all lines.
	If -b, print line numbers on all non-blank lines.
*/
void print_line_numbers(CatOpts *options, CatState *state) {
	// Not a blank line
	if(options->line_number_no_blank && state->input != '\n') {
		printf("%6d  ", state->current_line);
		state->current_line++;
	} else if(!options->line_number_no_blank) {
		printf("%6d  ", state->current_line);
		state->current_line++;
	} else {
		// No printing
	}
}

/*
	Print the end of line if option is enabled.
*/
void print_end_of_line(CatState *state) {
	/*
		GNU cat prints $ on any line, even those not numbered. 
		(try out option combination -bE)
		My version of cat follows this behavior.
	*/
	printf("$");
}

/*
	If we want to print non-printing characters, we want to actually
	avoid converting the newline we enter, so we check to see if we
	entered it. If not, then we check to see if it is an unprintable
	character. If it is an unprintable character we will convert it
	by adding 0x41-1 to its character code to get a printing ASCII 
	character. If we want to show tabs, we do the same. Additionally,
	we print a ^ to indicate it is a control character.
*/
void print_non_printing_characters(CatOpts *options, CatState *state){
	if(state->input != '\n') {
		if(state->input < PRINTABLE_CHARACTERS_START) {
			if(options->show_tabs) {
				state->input += ALPHABET_START;
				state->modified_char = 1;
				printf("^");
			} else {
				if(state->input != TAB) {
					state->input += ALPHABET_START;
					state->modified_char = 1;
					printf("^");
				}
			}
		}
	}
}

/*
	The main cat loop.

	Essentially a small state machine that keeps track of many attributes of
	the current state of cat's input.

	Pass the options you wish to invoke and the files to invoke it on.
	If files is empty, cat will read from standard input.
*/
void cat(CatOpts *options, char *files[]) {
	/* Setup our state variables. */
	CatState *state = new_CatState();
	int i;

	/* Allocate space for all our file handles. */	
	FILE **filehandles = malloc(sizeof(FILE) * (options->files+1));

	/* If we have files, open them for reading. Otherwise only open standard input. */	
	if(options->files != 0) {
		/* Open all the files */
		for(i = 0; i < options->files; i++) {
			filehandles[i] = fopen(files[i], "r");
		}
	} else {
		filehandles[0] = stdin;
		options->files = 1;
	}


	/* Parse all the files */
	for(i = 0; i < options->files; i++) {
		/* If one of the files couldn't be opened, go to the next one. */
		if(filehandles[i] == NULL)
			continue;
			
		state->current_line = 1;
	
		/* Loop until fgetc returns an EOF signal. */		
		while((state->input = fgetc(filehandles[i])) != EOF) {
			/* Reset if a character has been modified so that it may be printed. */
			state->modified_char = 0;

			/* If we have encountered a newline, we're at the end of a line. */
			if(state->input == '\n') {
				state->end_of_line = 1;
			} else {
				state->end_of_line = 0;
				state->multiple_newlines = 0;
			}

			/*
				If there are no options specified, avoid going through all the conditions.
			*/
			if(!options->no_options) {
				if(options->suppress_many_blank_lines) {
					suppress_output(state);
				}

				/* If output is suppressed any of these options are ignored. */
				if(!state->suppress_output) {
					/*
						If we're at the beginning of a line, (possibly, depends on options) 
						print a line number.
					*/
					if(state->beginning_of_line) {
						if(options->line_numbers) {
							print_line_numbers(options, state);
						}
					} 
					/*
						If we're at the end of a line, check to see if we want to
						visually indicate that. If so print a $.
					*/
					if(state->end_of_line) {
						if(options->end_of_line) {
							print_end_of_line(state);
						}
					}  
					
					if(options->show_non_printing) {
						print_non_printing_characters(options, state);
					}
				}
			}
			/*
				If we don't want to suppress output, print our input character to standard
				output. If there are any errors we will stop reading from this file.
			*/
			if(!state->suppress_output) {
				if(fputc(state->input, stdout) == EOF) {
					break; // There was an error writing to stdout.
				}
			}
	
			/*
				If our input character is a newline, and we are done processing, that means
				the next time around we are at the beginning of a new line!
			*/	
			if(state->input == '\n') {
				state->beginning_of_line = 1;
			} else {
				state->beginning_of_line = 0;
			}

			/*
				If our last 2 characters were newlines, then we record that we've gone by
				multiple newlines.
			*/
			if(state->input == '\n' && state->last_char == '\n') {
				state->multiple_newlines = 1;
			} else {
				state->multiple_newlines = 0;
			}

			/* Clean up */
			state->suppress_output = 0;
			state->last_char = state->input;

			if(state->modified_char) {
				state->last_char -= ALPHABET_START;
			}
		}
	}

	/* If we opened any files then we're sure to close them! */	
	if(options->files != 0) {
		/* Close all the files */
		for(i = 1; i < options->files; i++) {
			fclose(filehandles[i]);
		}
	}
}

/*
	Main entry point. Take user's arguments and run them through cat.
*/
int main(int argc, char *argv[]) {
	CatOpts *options = parse_cat_options(argc, argv);
	cat(options, argv+options->files_start);
	
	return 0;
}

