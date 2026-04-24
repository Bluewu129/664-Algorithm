/* compile with:
   gcc -O2 -o brute-force_text_reconstruction brute-force_text_reconstruction.c
*/

#include <stdio.h>     // getline(), fputs(), fprintf(), fopen(), stdin, stdout, stderr
#include <stdlib.h>    // malloc(), free()
#include <string.h>    // strcmp()
#include <assert.h>    // assert()

struct fragment_s {            // datatype "struct fragment_s" for the elements of a singly linked list.
  struct fragment_s * next_fragment;
  char * fragment_string;
};

struct fragment_s *
read_all_fragments( char const * file_name )
{
  FILE * input = stdin;                                                   // read from standard input if file_name is "-",
  if( strcmp( file_name, "-" ) != 0 )
    input = fopen( file_name, "r" );                                       // otherwise open the named file for "r"eading.
  if( input == NULL ) {
    fprintf( stderr, "Error: file could not be opened for reading: %s\n", file_name );
    return NULL;
  }
  struct fragment_s * top_fragment = NULL;    // prepate a singly linked list to hold the text fragments, initially empty.
  while( 1 ) {
    char * line_buffer = NULL;           // memory buffer for the next line read from input, to be allocated by getline().
    size_t buffer_size = 0;            // this variable will be set to the size of the line buffer allocated by getline().
    ssize_t read_bytes = getline( & line_buffer, & buffer_size, input );     // allocate buffer and store next input line.
    if( read_bytes <= 0 ) {
      if( line_buffer != NULL ) free( line_buffer );  // 'man getline' says the caller must free the buffer even on error.
      break;           // break from the input read loop as soon as nothing else can be read, such as at the end of input.
    }
    assert( line_buffer != NULL );    // ensure getline() did allocate a fresh buffer and save its address in line_buffer.
    assert( line_buffer[read_bytes] == '\0' );  // emsure the getline()-allocated string ends with extra '\0' as promised.
    int i = 0;
    for( i = 0; i < read_bytes; i ++ )  // some sanitising required: since getline() saves any read newline in the buffer,
      if( line_buffer[i] == '\n' || line_buffer[i] == '\r' )       // we replace it (on Linux/MacOS/Windows) with an '\0'.
        line_buffer[i] = '\0';
    struct fragment_s * new_fragment = malloc( sizeof( struct fragment_s ));
    new_fragment->next_fragment = top_fragment;
    new_fragment->fragment_string = line_buffer;                                                   // passing the baton...
    line_buffer = NULL;                                                                           // don't keep the baton!
    top_fragment = new_fragment;
#ifdef DEBUG
printf( "%ld: (%ld) %s\n", read_bytes, buffer_size, top_fragment->fragment_string );
#endif
  }
  fclose( input );
  input = NULL;
  return top_fragment;
}

void
free_all_fragments( struct fragment_s * top_fragment )
{
  while( top_fragment != NULL ) {
    struct fragment_s * this_fragment = top_fragment;
    top_fragment = this_fragment->next_fragment;
    free( this_fragment->fragment_string );
    free( this_fragment);
  }
}

bool
brute_force_string_matching( int length, char const * corpus, char const * pattern )
{
  int i, j;
  for( i = 0; i <= length ; i ++ ) {                   // we allow <= not just < so that zero-length matches are accepted.
    for( j = 0; pattern[j] != '\0'; j ++ )
      if( i+j == length || corpus[i+j] != pattern[j] )                                // mismatch for this start position.
        break;
    if( pattern[j] == '\0' )                                                           // complete match for this pattern.
      break;
  }
  if( i > length )                                                                    // no match at all for this pattern.
    return false;
  return true;
}

bool
iterate_next_string_of_given_size( int length, char * string )     // function side-effects: it edits the string directly.
{
  for( int i = 0; i < length; i ++ ) {        // enumerate strings by examining bytes from the left (an arbitrary choice),
    string[i] ++;                         // going through every possibly byte value! (ideally we'd skip non-printable...) 
    if( string[i] != '\0' )                                  // if the byte we just incremented did NOT roll over to '\0',
      return true;                                           // then the string is a novel string and we can return trure.
  }
  return false;                               // if we rolled over the whole set of strings for this length, return false.
}

int
main( int argc, char * argv[] )
{
  if( argc != 2 ) {
    fprintf( stderr, "Usage: %s [ <input_file> | - ]\n with input_file (or standard input if -) containing all string fragments on successive lines (no blanks)\n", argv[0] );
    exit( 1 );
  }
  struct fragment_s * top_fragment = read_all_fragments( argv[1] );   // build a linked list of fragments read from input.
  long int solutions = 0;
  int search_length = 0;
  while( solutions == 0 ) {         // outer iterator keeps increasing search_length until we found at least one solution.
    fprintf( stderr, "Searching for solution(s) of length %d...\n", search_length );
    char * search_string = NULL;
    search_string = malloc( search_length + 1 );    // allocate a buffer for the current search length plus one '\0' byte.
    assert( search_string != NULL );
    for( int i = 0; i <= search_length; i ++ ) {
      search_string[i] = '\0';             // initialise search_string to all '\0', including an extra end-of-string '\0'.
    }
    while( 1 ) {                          // huge inner iterator that enumerates every search_string of set search_length.
      struct fragment_s * test_fragment = top_fragment;
      while( test_fragment != NULL &&                                            // search for a match with all fragments.
             brute_force_string_matching( search_length, search_string, test_fragment->fragment_string )) {
        test_fragment = test_fragment->next_fragment;
      }
      if( test_fragment == NULL ) {               // all fragments were matches, so the search_string is a valid solution:
        fputs( search_string, stdout );                       // we output it and record success, but continue the search,
        solutions ++;                                                   // to find all solutions of the same minimum size.
      }
      bool novel = iterate_next_string_of_given_size( search_length, search_string );
      if( ! novel )                             // if no more novel string of search_length, break out of the enumeration.
        break;
    }
    free ( search_string );
    search_string = NULL;
    search_length ++;
  }
  fprintf( stderr, "Found %ld solution(s) of minimum size %d.\n", solutions,  search_length - 1 );
  free_all_fragments( top_fragment );
  top_fragment = NULL;
  return 0;                                                     // C convention is that main() should return 0 on success.
}

