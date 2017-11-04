
#include <stdlib.h>
#include <stdio.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <string.h>

//struct Library * DOSBase = NULL;
//struct DOSIFace *IDOS = NULL;

char *string[1000];
int string_count = 0;

char *exclude[1000];
int exclude_count = 0;

char *include[] = { ".c",".C",".h",".H",NULL };
char *opts[] ={ "-R", NULL };

char *bigbuffer;	
int bigbuffer_size = 1024 * 1024 ; // 1MB

BOOL is_correct_file( char *name )
{
	char **i;
	int name_len = strlen(name);
	int ext_len;

	for (i = include; *i ; i++)
	{
		ext_len =  strlen( *i );
		if (name_len - ext_len > 0)
		{
			if (strcmp(name + name_len - ext_len, *i) == 0)	return TRUE;
		}
	}
	return FALSE;
}

void replace_text_in_file( char *name, char *from, char *to )
{
	int size;
	char *outfile;
 	FILE *fd;
	FILE *fdout = NULL;
	char *at;
	int from_len = strlen(from);

	BOOL changed = FALSE;

	size = strlen(name) + 4 + 1;
	if (size>0)
	{
		outfile = malloc( size );
		if (outfile)
		{
			sprintf(outfile,"%s%s",name,".out");
			fd = fopen(name,"r");

			if (fd) fdout = fopen(outfile,"w");
			if ((fd)&&(fdout))
			{
				while ( ! feof(fd) )
				{
					if (fgets( bigbuffer,bigbuffer_size,fd ))
					{
						at = strstr( bigbuffer, from );

						if (at)
						{
							*at = 0;
							fprintf(fdout,"%s", bigbuffer);
							fprintf(fdout,"%s", to);
							fprintf(fdout,"%s", at + from_len );
							changed = TRUE;
						}
						else
						{
							fprintf(fdout,"%s", bigbuffer);
						}
					}		
				}
			}
			if (fd) fclose(fd);
			if (fdout) fclose(fdout);
			
			if (changed)
			{
				printf("File %s changed\n",name);
				remove(name);
				rename(outfile, name);
			}

			free(outfile);
		}
	}
}

int32 SafeAddPart(char **oldPart, char *newPart)
{
	int32 success;
	int size;
	char *tmp;
	size = strlen( *oldPart ? *oldPart : "" ) + strlen(newPart) + 2;	// maybe need a / or : , need a null at end of the string.
	tmp = malloc( size );

	if (tmp)
	{
		sprintf(tmp, "%s", *oldPart ? *oldPart : "" );
		success = AddPart( tmp, newPart, size );

		if (*oldPart) free(*oldPart);
		*oldPart = tmp;
	}
	return success;
}

int recursive( char *dir )
{
	char *recursive_path = strdup(dir);

	APTR context = ObtainDirContextTags(EX_StringNameInput,dir,
	                       EX_DataFields,(EXF_NAME|EXF_LINK|EXF_TYPE),
	                       TAG_END);
	if( context )
	{
	    struct ExamineData *dat;

	    while((dat = ExamineDir(context)))  /* until no more data.*/
	    {
	        if( EXD_IS_LINK(dat) ) /* all links - check for these first ! */
	        {
	            if( EXD_IS_SOFTLINK(dat) )        /* a FFS style softlink */
	            {
	                Printf("softlink=%s points to %s\n",
	                                           dat->Name, dat->Link);
	            }
	            else                              /* hard or alt link to */
	            {                                 /* a file or directory */
	                Printf("hardlink=%s points to %s\n", 
	                                           dat->Name, dat->Link);
	            }
	        }
	        else if( EXD_IS_FILE(dat) )           /* a file */
	        {
			if (is_correct_file (dat->Name) )
			{
				char *path_and_file = strdup( recursive_path );
				
				if (path_and_file)
				{
					if (SafeAddPart( &path_and_file, dat->Name  ))
					{
						Printf("filename=%s\n",  path_and_file ); 

						replace_text_in_file( path_and_file, string[0], string[1] );
					}
					free(path_and_file);
				}
			}
	        }
	        else if ( EXD_IS_DIRECTORY(dat) )     /* a directory */
	        {
			if (SafeAddPart( &recursive_path, dat->Name  ))
			{
				recursive( recursive_path );
				sprintf( recursive_path, dir );	// restore dir
			}
	        }
	    }
	    if( ERROR_NO_MORE_ENTRIES != IoErr() )
	    {
	        PrintFault(IoErr(),NULL); /* failure - why ? */
	    }
	}
	else
	{
	    PrintFault(IoErr(),NULL);     /* failure - why ? */
	}

	ReleaseDirContext(context);             /* NULL safe */

	if (recursive_path) free(recursive_path);
}

int main(int args, char **arg)
{
	char **opt;
	int n=0;
	int string_count = 0;
	int opt_count = 0;
	BOOL recursive_opt = FALSE;

	bigbuffer = malloc (bigbuffer_size);
	if (!bigbuffer) return 0;

	for (n=1; n<args; n++)
	{
		if (*arg[n]=='-')
		{
			opt_count = 0;
			for (opt = opts; *opt; opt++ )
			{
				if (strcmp(arg[n],*opt)==0)
				{
					switch( opt_count )
					{
						case 0: 	recursive_opt= TRUE;
								break;
						default: 
								printf("found %d but ignored\n",arg[n]);
								break;
					}
				}
				opt_count ++;
			}
		}
		else
		{
			string[string_count] = arg[n];
			string_count ++;
		}
	}

	if (string_count == 2)
	{
		if ((strlen(string[0])>0)&&(recursive_opt == TRUE))
		{
			recursive("");
		}
		
		if (strlen(string[0])==0)
		{
			printf("ERROR: rpl can't repalce nothing\n");
		}

		if (recursive_opt == FALSE)
		{
			printf("ERROR: This light version only works with -R option enabled\n");
		}
	}
	else
	{
		printf("wrong number of strings\n");
	}

	return  0;
}

