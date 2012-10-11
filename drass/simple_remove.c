#define _LARGEFILE_SOURCE
#define _LARGEFILE64_SOURCE
#define _FILE_OFFSET_BITS 64
#include <math.h>
#include <time.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/*-------------------------------------*/
//for file mapping in Linux
#include<fcntl.h>
#include<unistd.h>
#include<sys/stat.h>
#include<sys/time.h>
#include<sys/mman.h>
#include<sys/types.h>
/*-------------------------------------*/
#include "bloom.h"
#include "hashes.h"
#include "file_dir.h"
#include "tool.h"
/*-------------------------------------*/
//openMP library
#include<omp.h>
//#include<mpi.h>
/*-------------------------------------*/
#define PERMS 0600
#define NEW(type) (type *) malloc(sizeof(type))
/*-------------------------------------*/
float error_rate, tole_rate, contamination_rate;
/*-------------------------------------*/
int k_mer, mode, mytask, ntask, type = 2;
/*-------------------------------------*/
char *source, *all_ref, *position, *prefix, *clean, *contam, *clean2,
  *contam2, *list;
/*-------------------------------------*/
Queue *head, *tail, *head2;
/*-------------------------------------*/
bloom *bl_2;
/*-------------------------------------*/
F_set *File_head;
/*-------------------------------------*/
void struc_init ();
void get_parainfo (char *full);
void get_size (char *strFileName);
void init (int argc, char **argv);
void fasta_process (bloom * bl, Queue * info);
void fastq_process (bloom * bl, Queue * info);
void save_result (char *source, char *obj_file);
/*-------------------------------------*/
//char *mmaping (char *source);
/*-------------------------------------*/
/*
int fastq_full_check (bloom * bl, char *p, int distance,  char *model);
int fasta_full_check (bloom * bl, char *begin, char *next, char *model);
int fastq_read_check (char *begin, int length, char *model, bloom * bl);
int fasta_read_check (char *begin, char *next, char *model, bloom * bl);
*/
/*-------------------------------------*/
main (int argc, char **argv)
{

  long sec, usec, i;

  struct timezone tz;

  struct timeval tv, tv2;

  gettimeofday (&tv, &tz);

  init (argc, argv);		//initialize 

  struc_init ();		//structure init   

  if (strstr (source, ".fifo"))
    position = large_load (source);
  else
    position = mmaping (source);

  get_parainfo (position);

  clean = (char *) malloc (strlen (position) * sizeof (char));
  contam = (char *) malloc (strlen (position) * sizeof (char));
  clean2 = clean;
  contam2 = contam;

  while (File_head)
    {

      memset (clean2, 0, strlen (position));
      memset (contam2, 0, strlen (position));

      load_bloom (File_head->filename, bl_2);
      k_mer = bl_2->k_mer;

#pragma omp parallel
      {
#pragma omp single nowait
	{
	  while (head != tail)
	    {
#pragma omp task firstprivate(head)
	      {
		if (head->location)
		  if (type == 1)
		    fasta_process (bl_2, head);
		  else
		    fastq_process (bl_2, head);
	      }
	      head = head->next;
	    }
	}			// End of single - no implied barrier (nowait)
      }				// End of parallel region - implied barrier

      save_result (source,File_head->filename);

      File_head = File_head->next;

      head = head2;

      bloom_destroy (bl_2);

    }				//end while

  munmap (position, strlen (position));

  printf ("finish processing...\n");

  gettimeofday (&tv2, &tz);

  sec = tv2.tv_sec - tv.tv_sec;

  usec = tv2.tv_usec - tv.tv_usec;

  printf ("total=%ld sec\n", sec);

  return 0;
}

/*-------------------------------------*/
void
init (int argc, char **argv)
{
  if (argc == 1 || !strcmp (argv[1], "-h") || !strcmp (argv[1], "-help"))
    {
      help ();
      remove_help ();
    }
/*-------default-------*/
  mode = 1;
  tole_rate = 0.8;
  error_rate = 0.0005;
  prefix = NULL;
/*-------default-------*/
  int x;
  while ((x = getopt (argc, argv, "m:t:o:r:q:l:h")) != -1)
    {
      //printf("optind: %d\n", optind);
      switch (x)
	{
	case 'm':
	  //printf ("Mode : \nThe argument of -m is %s\n", optarg);
	  (optarg) && ((mode = atoi (optarg)), 1);
	  break;
	case 't':
	  //printf ("Tolerant rate: \nThe argument of -t is %s\n", optarg);
	  (optarg) && ((tole_rate = atof (optarg)), 1);
	  break;
	case 'o':
	  //printf ("Out : \nThe argument of -o is %s\n", optarg);
	  (optarg) && ((prefix = optarg), 1);
	  break;
	case 'r':
	  //printf ("Bloom list : \nThe argument of -r is %s\n", optarg);
	  (optarg) && ((all_ref = optarg), 1);
	  break;
	case 'q':
	  //printf ("Query : \nThe argument of -q is %s\n", optarg);
	  (optarg) && (source = optarg, 1);
	  break;
	case 'l':
	  (optarg) && (list = optarg, 1);
	  break;
	case 'h':
	  help ();
	  remove_help ();
	  break;
	case '?':
	  printf ("Unknown option: -%c\n", (char) optopt);
	  exit (0);
	}

    }

  if (((!all_ref) && (!list)) || (!source))
    {
      perror ("No source.");
      exit (0);
    }

  if (mode != 1 && mode != 2)
    {
      perror ("Mode select error.");
      exit (0);
    }

}

/*-------------------------------------*/
void
struc_init ()
{
  bl_2 = NEW (bloom);
  head = NEW (Queue);
  tail = NEW (Queue);
  head->next = tail;
  head2 = head;
  File_head = NEW (F_set);
  File_head = make_list (all_ref, list);
  File_head = File_head->next;

}
/*-------------------------------------*/
void
get_parainfo (char *full)
{
  printf ("distributing...\n");

  char *temp = full;

  int cores = omp_get_num_procs ();

  int offsett = strlen (full) / cores;

  int add = 0;

  printf ("task->%d\n", offsett);

  Queue *pos = head;

  if (*full == '>')
    type = 1;
  else if (*full == '@')
    type = 2;
  else
    {
      perror ("wrong format\n");
      exit (-1);
    }


  if (type == 1)
    {
      for (add = 0; add < cores; add++)
	{
	  Queue *x = NEW (Queue);

	  if (add == 0 && *full != '>')

	    temp = strchr (full, '>');	//drop the possible fragment

	  if (add != 0)

	    temp = strchr (full + offsett * add, '>');

	  //printf ("full->%0.20s\n", full);

	  x->location = temp;

	  x->number = add;

	  x->next = pos->next;

	  pos->next = x;

	  pos = pos->next;
	}
    }				// end if

  else
    {
      for (add = 0; add < cores; add++)
	{
	  Queue *x = NEW (Queue);

	  if (add == 0 && *full != '@')

	    temp = strstr (full, "\n@") + 1;	//drop the fragment

	  //printf("offset->%d\n",offsett*add);

	  if (add != 0)

	    temp = strstr (full + offsett * add, "\n@");

	  if (temp)
	    temp++;

	  x->location = temp;

	  x->number = add;

	  x->next = pos->next;

	  pos->next = x;

	  pos = pos->next;
	}			//end else  

    }

  return;
}

/*-------------------------------------*/
void
fastq_process (bloom * bl, Queue * info)
{
  printf ("fastq processing...\n");

  int read_num = 0;
  char *p = info->location;
  char *next, *temp_start, *temp_end, *temp_piece = NULL;

  if (info->next == NULL)
    return;

  else if (info->next != tail)
    next = info->next->location;

  else
    next = strchr (p, '\0');

  while (p != next)
    {

      read_num++;

      temp_start = p;

      if (p == '\0' || p == NULL)
	break;

      p = strchr (p, '\n') + 1;

      temp_end = strstr (p, "\n@");

      if (!temp_end)
	temp_end = strchr (p, '\0');
      int result = fastq_read_check (p, strchr (p, '\n') - p, "normal", bl, tole_rate);

      if (result == 0)
	{
#pragma omp critical
	  {
	    memcpy (clean, temp_start, temp_end - temp_start);
	    clean += (temp_end - temp_start);
	    if (*temp_end != '\0')
	      {
		clean[0] = '\n';
		clean++;
	      }
	  }
	}
      else if (result > 0)
	{
#pragma omp critical
	  {
	    //printf("in\n");
	    memcpy (contam, temp_start, temp_end - temp_start);
	    //printf("??\n");
	    contam += (temp_end - temp_start);
	    if (*temp_end != '\0')
	      {
		contam[0] = '\n';
		contam++;
	      }
	  }
	}


      if (*temp_end == '\0')
	break;

      p = temp_end + 1;

    }				// outside while
//free(key);
  if (temp_piece)
    free (temp_piece);
}
/*-------------------------------------*/
void
fasta_process (bloom * bl, Queue * info)
{
  printf ("fasta processing...\n");

  int read_num = 0, read_contam = 0;

  char *p = info->location;

  char *next;

  char *temp = p;

  if (info->next == NULL)
    return;
  else if (info->next != tail)
    next = info->next->location;
  else
    next = strchr (p, '\0');

  while (p != next)
    {
      read_num++;
      temp = strchr (p + 1, '>');
      if (!temp)
	temp = next;
	  
      int result = fasta_read_check (p, temp, "normal", bl, tole_rate);
      if (result==0)
	{
#pragma omp critical
	  {
	    memcpy (clean, p, temp - p);
	    clean += (temp - p);
	  }
	}
      else if (result > 0)
	{
#pragma omp atomic
	  read_contam++;
#pragma omp critical
	  {
	    memcpy (contam, p, temp - p);
	    contam += (temp - p);
	  }
	}
      p = temp;
    }
  printf ("all->%d\ncontam->%d\n", read_num, read_contam);
}

/*-------------------------------------*/
void
save_result (char *source, char *obj_file)
{
  printf ("saving...\n");
  char *match = (char *) malloc (400 * sizeof (char)),
    *mismatch = (char *) malloc (400 * sizeof (char)),
    *so_name = (char *) malloc (200 * sizeof (char)),
    *obj_name = (char *) malloc (200 * sizeof (char));

  memset (match, 0, 200);
  memset (mismatch, 0, 200);
  memset (so_name, 0, 200);
  memset (obj_name, 0, 200);

  char *so;
  ((so = strrchr (source, '/'))) && (so += 1, 1) || (so = NULL);

  char *obj;
  ((obj = strrchr (obj_file, '/'))) && (obj += 1, 1) || (obj = NULL);

  if (so)
    strncat (so_name, so, strrchr (source, '.') - so);
  else
    strncat (so_name, source, strrchr (source, '.') - source);

  if (obj)
    strncat (obj_name, obj, strrchr (obj_file, '.') - obj);
  else
    strncat (obj_name, obj_file, strrchr (obj_file, '.') - obj_file);

  if (prefix)
    {
      strcat (match, prefix);
      strcat (mismatch, prefix);
    }
  else if (so)
    {
      strncat (match, source, so - source);
      strncat (mismatch, source, so - source);
    }
  //printf ("objname->%s\n",obj_name);
  //printf ("match->%s\n", match);
  //printf ("mismatch->%s\n", mismatch);
  strcat (match, so_name);
  strcat (mismatch, so_name);
  //printf ("match->%s\n", match);
  //printf ("mismatch->%s\n", mismatch);
  strcat (match, "_");
  strcat (mismatch, "_");
  //printf ("match->%s\n", match);
  //printf ("mismatch->%s\n", mismatch);
  strcat (match, obj_name);
  strcat (mismatch, obj_name);
  //printf ("match->%s\n", match);
  //printf ("mismatch->%s\n", mismatch);
  strcat (match, "_contam");
  strcat (mismatch, "_clean");

  if (type == 1)
    {
      strcat (match, ".fasta");
      strcat (mismatch, ".fasta");
    }
  else
    {
      strcat (match, ".fastq");
      strcat (mismatch, ".fastq");
    }
  printf ("match->%s\n", match);
  printf ("mis->%s\n", mismatch);

  write_result (match, contam2);

  write_result (mismatch, clean2);

  free (match);

  free (mismatch);

  free (so_name);

  free (obj_name);

  memset (contam2, 0, strlen (contam2));

  memset (clean2, 0, strlen (clean2));

  clean = clean2;

  contam = contam2;

}
