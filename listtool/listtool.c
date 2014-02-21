/* listtool is a simple list manipulation utility for Pd */
/* Copyright David Medine */
/* released under the terms of the GPL */


#include "m_pd.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <float.h>

/* -------------------------- listtool ------------------------------ */
static t_class *listtool_class;

typedef struct _listtool
{
  t_object x_obj;
  t_outlet *info;
  t_outlet *list;
  t_outlet *indeces;
  t_symbol *x_arrayname;
  t_word *x_vec;
  int arraypts;
  int rand_state;
  t_atom *list2v;
  int list2c;
} t_listtool;

//function defs:
static t_float listtool_sum(t_listtool *x, t_symbol *s, int argc, t_atom *argv);

//the methods:
static void listtool_size(t_listtool *x, t_symbol *s, int argc, t_atom *argv)
{

  outlet_float(x->info, argc); 
  
}

static void listtool_normalize(t_listtool *x, t_symbol *s, int argc, t_atom *argv)
{
  t_atom *outv;
  t_float *temp;
  int outc = argc-2;
  int i; 
  t_float y; 
  t_float range, min, max;

  min = atom_getfloat(argv);
  max = atom_getfloat(argv+1);
 
  if(min>max)
    {
      y=min;
      min=max;
      max=y;
    }

  range= max-min;
  
  //setup local memory
  outv = (t_atom *)getbytes(0);
  outv = (t_atom *)resizebytes(outv, 0, outc * sizeof(t_atom));
  
  //put list into temp buffer  
  for(i = 0; i<outc; i++)
    SETFLOAT(outv+i, atom_getfloat(argv+i+2));

  //tool the list:
  //first, find the smallest value, and shift accordingly:
  y=atom_getfloat(outv);
  for(i = 1; i<outc; i++)
    if(y<atom_getfloat(outv+i));
    else 
      y=atom_getfloat(outv+i);
  
  for(i=0; i<outc; i++)
    SETFLOAT(outv+i, atom_getfloat(outv+i)-y);

  //second find largest value (y)
  y = atom_getfloat(outv);
  for(i = 1; i<outc; i++)
    if(y>atom_getfloat(outv+i));
    else 
      y=atom_getfloat(outv+i);

  //make sure that we don't divide by zero
  if(y == 0.0) goto output;
  
  //third divide all values by the largest value (y)
  for(i = 0; i<outc; i++)
    SETFLOAT(outv+i, (atom_getfloat(outv+i)/y));

  //now rescale to range
  for(i = 0; i<outc; i++)
    SETFLOAT(outv+i,atom_getfloat(outv+i) * range);

  //finally, offset to the min value
  for(i = 0; i<outc; i++)
    SETFLOAT(outv+i, atom_getfloat(outv+i)+min);
 
  //output 
 output:
  outlet_list(x->list, 0, outc, outv);
  
  //free memory
  freebytes(outv, outc * sizeof(t_atom));

}

static void listtool_normalize_sum(t_listtool *x, t_symbol *s, int argc, t_atom *argv)
{
  t_atom *outv;
  t_float *temp;
  int outc = argc-1;
  int i; 
  t_float y; 
  t_float sum;
  t_float total = atom_getfloat(argv);  

  //setup local memory
  outv = (t_atom *)getbytes(0);
  outv = (t_atom *)resizebytes(outv, 0, outc * sizeof(t_atom));
  
  //put list into temp buffer  
  for(i = 0; i<outc; i++)
    SETFLOAT(outv+i, atom_getfloat(argv+i+1));

  //tool the list:
  //first, find the smallest value, and shift accordingly:
  y=atom_getfloat(outv);
  for(i = 1; i<outc; i++)
    if(y<atom_getfloat(outv+i));
    else 
      y=atom_getfloat(outv+i);
  
  for(i=0; i<outc; i++)
    SETFLOAT(outv+i, atom_getfloat(outv+i)-y);
  
  //find the total sum
  sum = listtool_sum(x, 0, outc, outv);
  if(sum == 0.0) sum = 1.0;
  sum = sum/total;

  //normalize accordingly:
  for(i=0; i<outc; i++)
    SETFLOAT(outv+i, atom_getfloat(outv+i)/sum);

  //output
  outlet_list(x->list, 0, outc, outv);

  //free memory
  freebytes(outv, outc * sizeof(t_atom));
}

static void listtool_powtodb(t_listtool *x, t_symbol *s, int argc, t_atom *argv)
{
 int i;
  t_float value = 0.0;
  t_atom *outv;
  int outc = argc;

  outv = (t_atom *)getbytes(0);
  outv = (t_atom *)resizebytes(outv, 0, outc * sizeof(t_atom));

  for(i=0; i<argc; i++)
    {
      value = (atom_getfloat(argv+i));//set value to the current item in the list
      value=powtodb(value);//calculate the db 
      SETFLOAT(outv+i, value);
    }
  outlet_list(x->list, 0, outc, outv);

  freebytes(outv, outc * sizeof(t_atom));
}


static void listtool_rmstodb(t_listtool *x, t_symbol *s, int argc, t_atom *argv)
{
 int i;
  t_float value = 0.0;
  t_atom *outv;
  int outc = argc;

  outv = (t_atom *)getbytes(0);
  outv = (t_atom *)resizebytes(outv, 0, outc * sizeof(t_atom));

  for(i=0; i<argc; i++)
    {
      value = (atom_getfloat(argv+i));//set value to the current item in the list
      value=rmstodb(value);//calculate the db 
      SETFLOAT(outv+i, value);
    }
  outlet_list(x->list, 0, outc, outv);

  freebytes(outv, outc * sizeof(t_atom));
}



static void listtool_min(t_listtool *x, t_symbol *s, int argc, t_atom *argv)
{
  int i;
  t_float min;
  t_atom *outv;
  int outc = 1;

  outv = (t_atom *)getbytes(0);
  outv = (t_atom *)resizebytes(outv, 0, outc * sizeof(t_atom));

  min = FLT_MAX;
  for(i=0; i<argc; i++)
    if((atom_getfloat(argv+i))<min)
      {
	min = atom_getfloat(argv+i);
	SETFLOAT(outv, i);
      }

  outlet_float(x->info, min);
  outlet_list(x->indeces, 0, 1, outv);
  
  freebytes(outv, outc * sizeof(t_atom));


}

static void listtool_max(t_listtool *x, t_symbol *s, int argc, t_atom *argv)
{
  int i;
  t_float max;
  t_atom *indecesv;
  int indecesc = 1;

  indecesv = (t_atom *)getbytes(0);
  indecesv = (t_atom *)resizebytes(indecesv, 0, indecesc * sizeof(t_atom));

  max = -1 * FLT_MAX;
  for(i=0; i<argc; i++)
    if((atom_getfloat(argv+i))>max)
      {
	max = atom_getfloat(argv+i);
	SETFLOAT(indecesv, i);
      }

  outlet_float(x->info, max);
  outlet_list(x->indeces, 0, 1, indecesv);
  
  freebytes(indecesv, indecesc * sizeof(t_atom));


}

static void listtool_nearest(t_listtool *x, t_symbol *s, int argc, t_atom *argv)
{
  int i, index;
  t_float nearest, distance;
  t_atom *indecesv;
  int indecesc = 1;

  indecesv = (t_atom *)getbytes(0);
  indecesv = (t_atom *)resizebytes(indecesv, 0, indecesc * sizeof(t_atom));

  nearest = atom_getfloat(argv);
  distance = FLT_MAX;
  index = 0;

  //start at the second index, because the first value is 'nearest'
  for(i=1; i<argc; i++)
    if(fabs((atom_getfloat(argv+i)) - nearest)<distance)
      {
	distance = fabs((atom_getfloat(argv+i))-nearest);
	SETFLOAT(indecesv, (i-1));
	index = i;
      }

 
  outlet_float(x->info, atom_getfloat(argv+index));
  outlet_list(x->indeces, 0, indecesc, indecesv);
 
  freebytes(indecesv, indecesc * sizeof(t_atom));
}

static void listtool_equals(t_listtool *x, t_symbol *s, int argc, t_atom *argv)
{
  int i, j, matches; 
  t_float equiv;
  t_atom *indecesv;
 
  indecesv = (t_atom *)getbytes(0);
  equiv = atom_getfloat(argv);
  matches = j = 0;

  for(i=1; i<argc; i++)
    if(atom_getfloat(argv+i) == equiv)
      matches++;
  indecesv = (t_atom *)resizebytes(indecesv, 0, matches * sizeof(t_atom));

  for(i=1; i<argc; i++)
    if(atom_getfloat(argv+i) == equiv)
      {
	SETFLOAT((indecesv+j), (i-1));
	j++;
      }

  outlet_float(x->info, matches);
  outlet_list(x->indeces, 0, matches, indecesv);

 freebytes(indecesv, matches * sizeof(t_atom));
}

static void listtool_greater(t_listtool *x, t_symbol *s, int argc, t_atom *argv)
{
  int i, j, matches;
  t_float thresh;
  t_atom *indecesv;
  t_atom *outv;
  int outc = argc-1;

  indecesv = (t_atom *)getbytes(0);
  outv = (t_atom *)getbytes(0);


  thresh = atom_getfloat(argv);
  matches = j = 0;
 
  for(i=1; i<argc; i++)
    if(atom_getfloat(argv+i) > thresh)
      matches++;

   indecesv = (t_atom *)resizebytes(indecesv, 0, matches * sizeof(t_atom));
   outv = (t_atom *)resizebytes(outv, 0, matches * sizeof(t_atom));

 for(i=1; i<argc; i++)
    if(atom_getfloat(argv+i) > thresh)
      {
	SETFLOAT((indecesv+j), (i-1));
	SETFLOAT((outv+j), atom_getfloat(argv+i));
	j++;
      } 
 outlet_float(x->info, matches);
 outlet_list(x->indeces, 0, matches, indecesv);
 outlet_list(x->list, 0, matches, outv);

 freebytes(indecesv, matches * sizeof(t_atom));
 freebytes(outv, matches * sizeof(t_atom));
}

static void listtool_greater_equal(t_listtool *x, t_symbol *s, int argc, t_atom *argv)
{
  int i, j, matches;
  t_float thresh;
  t_atom *indecesv;
  t_atom *outv;
  int outc = argc-1;

  indecesv = (t_atom *)getbytes(0);
  outv = (t_atom *)getbytes(0);


  thresh = atom_getfloat(argv);
  matches = j = 0;
 
  for(i=1; i<argc; i++)
    if(atom_getfloat(argv+i) >= thresh)
      matches++;

   indecesv = (t_atom *)resizebytes(indecesv, 0, matches * sizeof(t_atom));
   outv = (t_atom *)resizebytes(outv, 0, matches * sizeof(t_atom));

 for(i=1; i<argc; i++)
    if(atom_getfloat(argv+i) >= thresh)
      {
	SETFLOAT((indecesv+j), (i-1));
	SETFLOAT((outv+j), atom_getfloat(argv+i));
	j++;
      } 
 outlet_float(x->info, matches);
 outlet_list(x->indeces, 0, matches, indecesv);
 outlet_list(x->list, 0, matches, outv);

 freebytes(indecesv, matches * sizeof(t_atom));
 freebytes(outv, matches * sizeof(t_atom));
}

static void listtool_less(t_listtool *x, t_symbol *s, int argc, t_atom *argv)
{
  int i, j, matches;
  t_float thresh;
  t_atom *indecesv;
  t_atom *outv;
  int outc = argc-1;

  indecesv = (t_atom *)getbytes(0);
  outv = (t_atom *)getbytes(0);


  thresh = atom_getfloat(argv);
  matches = j = 0;
 
  for(i=1; i<argc; i++)
    if(atom_getfloat(argv+i) < thresh)
      matches++;

   indecesv = (t_atom *)resizebytes(indecesv, 0, matches * sizeof(t_atom));
   outv = (t_atom *)resizebytes(outv, 0, matches * sizeof(t_atom));

 for(i=1; i<argc; i++)
    if(atom_getfloat(argv+i) < thresh)
      {
	SETFLOAT((indecesv+j), (i-1));
	SETFLOAT((outv+j), atom_getfloat(argv+i));
	j++;
      } 
 outlet_float(x->info, matches);
 outlet_list(x->indeces, 0, matches, indecesv);
 outlet_list(x->list, 0, matches, outv);

 freebytes(indecesv, matches * sizeof(t_atom));
 freebytes(outv, matches * sizeof(t_atom));
}

static void listtool_less_equal(t_listtool *x, t_symbol *s, int argc, t_atom *argv)
{
  int i, j, matches;
  t_float thresh;
  t_atom *indecesv;
  t_atom *outv;
  int outc = argc-1;

  indecesv = (t_atom *)getbytes(0);
  outv = (t_atom *)getbytes(0);


  thresh = atom_getfloat(argv);
  matches = j = 0;
 
  for(i=1; i<argc; i++)
    if(atom_getfloat(argv+i) <= thresh)
      matches++;

   indecesv = (t_atom *)resizebytes(indecesv, 0, matches * sizeof(t_atom));
   outv = (t_atom *)resizebytes(outv, 0, matches * sizeof(t_atom));

 for(i=1; i<argc; i++)
    if(atom_getfloat(argv+i) <= thresh)
      {
	SETFLOAT((indecesv+j), (i-1));
	SETFLOAT((outv+j), atom_getfloat(argv+i));
	j++;
      } 
 outlet_float(x->info, matches);
 outlet_list(x->indeces, 0, matches, indecesv);
 outlet_list(x->list, 0, matches, outv);

 freebytes(indecesv, matches * sizeof(t_atom));
 freebytes(outv, matches * sizeof(t_atom));
}

static void listtool_choose(t_listtool *x, t_symbol *s, int argc, t_atom *argv)
{
  
  unsigned int randval;
  int i, j;
  int nval = i =  j = 0;
  t_atom *indecesv;

  indecesv = (t_atom *)getbytes(0);
  indecesv = (t_atom *)resizebytes(indecesv, 0, sizeof(t_atom));


      randval = x->rand_state;
      x->rand_state = randval = randval * 472940017 + 832416023; // from random in x_misc.c
      nval = ((double)(argc-1)) * ((double)randval)
	* (1./4294967296.);

      nval = (int)nval % (argc-1);
      SETFLOAT(indecesv, nval);

      outlet_float(x->info, atom_getfloat(argv + nval));

  outlet_list(x->indeces, 0, 1, indecesv);

  freebytes(indecesv, sizeof(t_atom));
}

static void listtool_const(t_listtool *x, t_symbol *s, int argc, t_atom *argv)
{
  int i, idx1, idx2, temp, diff, outc;
  t_float val;
  t_atom *outv;

  idx1 = (int)atom_getfloat(argv + 1);
  idx2 = (int)atom_getfloat (argv + 2);

  if(idx1>idx2)
    {
      temp=idx1;
      idx1=idx2;
      idx2=temp;
    }

  diff = idx2-idx1;
  outc = argc - 3 + diff;
  val = atom_getfloat (argv);
  
  outv = (t_atom *)getbytes(0);
  outv = (t_atom *)resizebytes(outv, 0, outc * sizeof(t_atom));

  for(i=0; i<idx1; i++)
    SETFLOAT(outv+i, atom_getfloat(argv+i+3));
  for(i=idx1; i<idx2; i++)
    SETFLOAT(outv+i, val);
  for(i=idx2; i<outc; i++)
    SETFLOAT(outv+i, atom_getfloat(argv-diff+3+i));

  outlet_list(x->list, 0, outc, outv);

  freebytes(outv, outc*sizeof(t_atom));
}

static void listtool_offset(t_listtool *x, t_symbol *s, int argc, t_atom *argv)
{
  t_float offset, newval;
  int i;

  t_atom *outv;
  int outc = argc-1;
  offset = atom_getfloat(argv);
  outv = (t_atom *)getbytes(0);
  outv = (t_atom *)resizebytes(outv, 0, outc * sizeof(t_atom));

  for(i=0; i<outc; i++)
    SETFLOAT(outv+i, atom_getfloat(argv+i+1)+offset);
  
  outlet_list(x->list, 0, outc, outv);  

  freebytes(outv, outc * sizeof(t_atom));
}

static void listtool_shift(t_listtool *x, t_symbol *s, int argc, t_atom *argv)
{
  int i, newindex;
  int shift = (int)atom_getfloat(argv);

  t_atom *outv;
  int outc = argc-1;

  outv = (t_atom *)getbytes(0);
  outv = (t_atom *)resizebytes(outv, 0, outc * sizeof(t_atom));

  for(i=0; i<outc; i++)
    {
      newindex = i+shift;
      if(newindex>=outc)newindex-=outc;
      if(newindex<0)newindex+=outc;
      SETFLOAT(outv+i, atom_getfloat(argv+1+newindex));
    }

  outlet_list(x->list, 0, outc, outv);  

  freebytes(outv, outc * sizeof(t_atom));
}

static void listtool_shift0(t_listtool *x, t_symbol *s, int argc, t_atom *argv)
{
  int i, newindex;
  int shift = (int)atom_getfloat(argv);

  t_atom *outv;
  int outc = argc-1;

  outv = (t_atom *)getbytes(0);
  outv = (t_atom *)resizebytes(outv, 0, outc * sizeof(t_atom));
  if (shift>=0)
    {   
      for(i=0; i<shift; i++)
	SETFLOAT(outv+i, 0);
      for(i=shift; i<outc; i++)
	SETFLOAT(outv+i, atom_getfloat(argv+i+1-shift));
    }
  else
    {
      for(i=outc+shift; i<outc; i++)
	SETFLOAT(outv+i, 0);
      for(i=0; i<outc+shift; i++)
	SETFLOAT(outv+i, atom_getfloat(argv+1+i-shift));
    }

  outlet_list(x->list, 0, outc, outv);  

  freebytes(outv, outc * sizeof(t_atom));
}

static void listtool_scale(t_listtool *x, t_symbol *s, int argc, t_atom *argv)
{
  t_float scalar = atom_getfloat(argv);
  int i;

  t_atom *outv;
  int outc = argc-1;

  outv = (t_atom *)getbytes(0);
  outv = (t_atom *)resizebytes(outv, 0, outc * sizeof(t_atom));

  for(i=0; i<outc; i++)
    SETFLOAT(outv+i, atom_getfloat(argv+i+1)*scalar);

  outlet_list(x->list, 0, outc, outv);  
  
  freebytes(outv, outc * sizeof(t_atom));
}

static void listtool_invert(t_listtool *x, t_symbol *s, int argc, t_atom *argv)
{
  int i;
  t_float min, max;

  t_atom *outv;
  int outc = argc;

  outv = (t_atom *)getbytes(0);
  outv = (t_atom *)resizebytes(outv, 0, outc * sizeof(t_atom));

  min= FLT_MAX;
  max = 0.0;

  for(i=0; i<outc; i++)
    {
      if(atom_getfloat(argv+i)>max)
	max=atom_getfloat(argv+i);
      if(atom_getfloat(argv+i)<min)
	min=atom_getfloat(argv+i);
    }

  for(i=0; i<outc; i++)
    SETFLOAT(outv+i, max - atom_getfloat(argv+i) + min);

  outlet_list(x->list, 0, outc, outv);  

  freebytes(outv, outc * sizeof(t_atom));	
}

static void listtool_reverse(t_listtool *x, t_symbol *s, int argc, t_atom *argv)
{ 
  int i; 
  t_atom *outv;
  int outc = argc;

  outv = (t_atom *)getbytes(0);
  outv = (t_atom *)resizebytes(outv, 0, outc * sizeof(t_atom));

  for(i=0; i<outc; i++)
    SETFLOAT(outv+i, atom_getfloat(argv+argc-1-i));

  outlet_list(x->list, 0, outc, outv);  

  freebytes(outv, outc * sizeof(t_atom));
}

static void listtool_remove(t_listtool *x, t_symbol *s, int argc, t_atom *argv)
{
  int i;

  t_atom *outv;
  int outc = argc-2;

  int removee = (int)atom_getfloat(argv);
  if(removee < 0)removee = 0;
  if(removee > outc)
      goto over;

  outv = (t_atom *)getbytes(0);
  outv = (t_atom *)resizebytes(outv, 0, outc * sizeof(t_atom));

  for(i=0; i<removee; i++)
    SETFLOAT(outv+i, atom_getfloat(argv+i+1));
  for(i=removee; i<outc; i++)
    SETFLOAT(outv+i, atom_getfloat(argv+i+2));

  outlet_list(x->list, 0, outc, outv);  

  freebytes(outv, outc * sizeof(t_atom));
  goto done;

 over:
  outlet_list(x->list, 0, argc-1, argv+1);
 done:;
}

static void listtool_remove0(t_listtool *x, t_symbol *s, int argc, t_atom *argv)
{
  int i;

  t_atom *outv;
  int outc = argc-1;

  int removee = (int)atom_getfloat(argv);
  if(removee < 0)removee = 0;
  if(removee > outc)
      goto over;

  outv = (t_atom *)getbytes(0);
  outv = (t_atom *)resizebytes(outv, 0, outc * sizeof(t_atom));

  for(i=0; i<removee; i++)
    SETFLOAT(outv+i, atom_getfloat(argv+i+1));
  for(i=removee; i<outc; i++)
    SETFLOAT(outv+i, atom_getfloat(argv+i+2));
  SETFLOAT(outv+outc-1, 0);

  outlet_list(x->list, 0, outc, outv);  

  freebytes(outv, outc * sizeof(t_atom));
  goto done;

 over:
  outlet_list(x->list, 0, argc-1, argv+1);
 done:;
}

static void listtool_integrate(t_listtool *x, t_symbol *s, int argc, t_atom *argv)
{
  int i;
  t_float sum;

  t_atom *outv;
  int outc = argc;

  outv = (t_atom *)getbytes(0);
  outv = (t_atom *)resizebytes(outv, 0, outc * sizeof(t_atom));

  SETFLOAT(outv, atom_getfloat(argv));
  sum = atom_getfloat(argv);
  for(i=1; i <argc; i++)
    {
      SETFLOAT(outv+i, atom_getfloat(argv+i) + sum);
      sum += atom_getfloat(argv+i);
    } 

  outlet_list(x->list, 0, outc, outv);  

  freebytes(outv, outc * sizeof(t_atom));
}

static void listtool_differentiate(t_listtool *x, t_symbol *s, int argc, t_atom *argv)
{
  int i;

  t_atom *outv;
  int outc = argc;

  outv = (t_atom *)getbytes(0);
  outv = (t_atom *)resizebytes(outv, 0, outc * sizeof(t_atom));

  SETFLOAT(outv, atom_getfloat(argv));
  for(i=1; i <argc; i++)
    SETFLOAT(outv+i, atom_getfloat(argv+i) - atom_getfloat(argv + i-1));
    

  outlet_list(x->list, 0, outc, outv);  

  freebytes(outv, outc * sizeof(t_atom));
}

static void listtool_add(t_listtool *x, t_symbol *s, int argc, t_atom *argv)
{
  if(!x->list2v)
    goto zeros;
  int i;
  int diff;
  int tag = 0;
  int outc;
  t_atom *outv;
  
  if(argc>=x->list2c) outc = argc;
  else
    {
      outc = x->list2c;
      tag = 1;
    }

  diff = fabs(x->list2c-argc);

  outv = (t_atom *)getbytes(0);
  outv = (t_atom *)resizebytes(outv, 0, outc * sizeof(t_atom));

  for(i=0; i<outc-diff; i++)
    SETFLOAT(outv+i, atom_getfloat(argv+i)+atom_getfloat(x->list2v+i));
  
  if(tag)
    {
    for(i=outc-diff; i<outc; i++)
      SETFLOAT(outv+i, atom_getfloat(x->list2v+i));
    }
  else
    {
    for(i=outc-diff; i<outc; i++)
      SETFLOAT(outv+i, atom_getfloat(argv+i));
    }

  outlet_list(x->list, 0, outc, outv);  

  freebytes(outv, outc * sizeof(t_atom));

  goto over;

 zeros:
  outlet_list(x->list, 0, argc, argv);

 over:;
}

static void listtool_subtract(t_listtool *x, t_symbol *s, int argc, t_atom *argv)
{
  if(!x->list2v)
    goto zeros;
  int i;
  int diff;
  int tag = 0;
  int outc;
  t_atom *outv;
  
  if(argc>=x->list2c) outc = argc;
  else
    {
      outc = x->list2c;
      tag = 1;
    }

  diff = fabs(x->list2c-argc);

  outv = (t_atom *)getbytes(0);
  outv = (t_atom *)resizebytes(outv, 0, outc * sizeof(t_atom));

  for(i=0; i<outc-diff; i++)
    SETFLOAT(outv+i, atom_getfloat(argv+i)-atom_getfloat(x->list2v+i));
  
  if(tag)
    {
    for(i=outc-diff; i<outc; i++)
      SETFLOAT(outv+i, atom_getfloat(x->list2v+i));
    }
  else
    {
    for(i=outc-diff; i<outc; i++)
      SETFLOAT(outv+i, atom_getfloat(argv+i));
    }

  outlet_list(x->list, 0, outc, outv);  

  freebytes(outv, outc * sizeof(t_atom));

  goto over;

 zeros:
  outlet_list(x->list, 0, argc, argv);

 over:;
}

static void listtool_multiply(t_listtool *x, t_symbol *s, int argc, t_atom *argv)
{
  if(!x->list2v)
    goto zeros;
  int i;
  int diff;
  int tag = 0;
  int outc;
  t_atom *outv;
  
  if(argc>=x->list2c) outc = argc;
  else
    {
      outc = x->list2c;
      tag = 1;
    }

  diff = fabs(x->list2c-argc);

  outv = (t_atom *)getbytes(0);
  outv = (t_atom *)resizebytes(outv, 0, outc * sizeof(t_atom));

  for(i=0; i<outc-diff; i++)
    SETFLOAT(outv+i, atom_getfloat(argv+i)*atom_getfloat(x->list2v+i));
  
  if(tag)
    {
    for(i=outc-diff; i<outc; i++)
      SETFLOAT(outv+i, atom_getfloat(x->list2v+i));
    }
  else
    {
    for(i=outc-diff; i<outc; i++)
      SETFLOAT(outv+i, atom_getfloat(argv+i));
    }

  outlet_list(x->list, 0, outc, outv);  

  freebytes(outv, outc * sizeof(t_atom));

  goto over;

 zeros:
  outlet_list(x->list, 0, argc, argv);

 over:;
}

static void listtool_divide(t_listtool *x, t_symbol *s, int argc, t_atom *argv)
{
  if(!x->list2v)
    goto zeros;
  int i;
  int diff;
  int tag = 0;
  int outc;
  t_atom *outv;
  
  if(argc>=x->list2c) outc = argc;
  else
    {
      outc = x->list2c;
      tag = 1;
    }

  diff = fabs(x->list2c-argc);

  outv = (t_atom *)getbytes(0);
  outv = (t_atom *)resizebytes(outv, 0, outc * sizeof(t_atom));

  for(i=0; i<outc-diff; i++)
    SETFLOAT(outv+i, (atom_getfloat(argv+i))/(atom_getfloat(x->list2v+i)));
  
  if(tag)
    {
    for(i=outc-diff; i<outc; i++)
      SETFLOAT(outv+i, atom_getfloat(x->list2v+i));
    }
  else
    {
    for(i=outc-diff; i<outc; i++)
      SETFLOAT(outv+i, atom_getfloat(argv+i));
    }

  outlet_list(x->list, 0, outc, outv);  

  freebytes(outv, outc * sizeof(t_atom));

  goto over;

 zeros:
  outlet_list(x->list, 0, argc, argv);

 over:;
}

static t_float listtool_sum(t_listtool *x, t_symbol *s, int argc, t_atom *argv)
{
  int i;
  t_float sum = atom_getfloat(argv);
  for(i=1; i<argc; i++)
    sum += atom_getfloat(argv+i);
  outlet_float(x->info, sum);
  return sum;
}

static float listtool_mean(t_listtool *x, t_symbol *s, int argc, t_atom *argv)
{
  t_float sum = listtool_sum(x, 0, argc, argv);
  t_float mean = sum/argc;
  outlet_float(x->info, mean);
  return(mean);
}

static void listtool_product(t_listtool *x, t_symbol *s, int argc, t_atom *argv)
{
  int i;
  t_float product = atom_getfloat(argv);
  for(i=1; i<argc; i++)
    product *= atom_getfloat(argv+i);
  outlet_float(x->info, product);
}

static void listtool_geomean(t_listtool *x, t_symbol *s, int argc, t_atom *argv)
{
  int i;
  t_float nthroots[argc], n_recip, geomean;

  geomean = 1.0;
  n_recip = 1.0/argc;

  for(i=0; i<argc; i++)
    nthroots[i]=pow(atom_getfloat(argv+i), n_recip);

  for(i=0; i<argc; i++)
    geomean *= nthroots[i];

  outlet_float(x->info, geomean);
}

static float listtool_std(t_listtool *x, t_symbol *s, int argc, t_atom *argv)
{
  t_float sum = listtool_sum(x, 0, argc, argv);
  t_float mean = sum/argc;
  sum = 0.0;
  t_float std;
  int i;
  t_float *outv;
  int outc = argc;

  outv = (t_float *)getbytes(0);
  outv = (t_float *)resizebytes(outv, 0, outc * sizeof(t_float));

  for(i=0; i<outc; i++)
    {
      outv[i] -=mean;
      outv[i] *= outv[i];
      sum+=outv[i];
    }
  std = sum/((t_float)outc-1.0);
  std = sqrt(std);

  outlet_float(x->info, std);

  freebytes(outv, outc * sizeof(t_float));

  return(std);
}

static void listtool_dot(t_listtool *x, t_symbol *s, int argc, t_atom *argv)
{
  if(!x->list2v)
    goto zeros;
  int i;
  int outc;
  t_float dot = 0;
  
  if(argc>=x->list2c) outc = argc;
  else
    outc = x->list2c;
    
  for(i=0; i<outc; i++)
    {
      dot+=(atom_getfloat(argv+i)*atom_getfloat(x->list2v+i));
    }

  outlet_float(x->info, dot);

  goto over;

 zeros:
  outlet_list(x->list, 0, argc, argv);

 over:;
}

static void listtool_euclid(t_listtool *x, t_symbol *s, int argc, t_atom *argv)
{
  if(!x->list2v)
    goto zeros;
  int i;
  int outc;
  t_float euc_diff = 0;
  t_float euclid = 0;
  
  if(argc>=x->list2c) outc = argc;
  else
    outc = x->list2c;
  
  for(i=0; i<outc; i++)
    {
      euc_diff=(atom_getfloat(argv+i)-atom_getfloat(x->list2v+i));
      euclid += euc_diff * euc_diff;
    }
 

  outlet_float(x->info, euclid);

  goto over;

 zeros:
  outlet_list(x->list, 0, argc, argv);

 over:;
}

static void listtool_taxi(t_listtool *x, t_symbol *s, int argc, t_atom *argv)
{
  if(!x->list2v)
    goto zeros;
  int i;
  int outc;
  t_float euc_diff = 0;
  t_float euclid = 0;
  
  if(argc>=x->list2c) outc = argc;
  else
    outc = x->list2c;
  
  for(i=0; i<outc; i++)
    {
      euc_diff=fabs(atom_getfloat(argv+i)-atom_getfloat(x->list2v+i));
      euclid += euc_diff;
    }
 

  outlet_float(x->info, euclid);

  goto over;

 zeros:
  outlet_list(x->list, 0, argc, argv);

 over:;
}

static void listtool_corr(t_listtool *x, t_symbol *s, int argc, t_atom *argv)
{
  if(!x->list2v)
    goto zeros;
  int centerc;
  t_atom *centerv, *center1v;
  int i;
  t_float mean, mean1;
  t_float std, std1;
  t_float corr;

  if(argc>=x->list2c) centerc = argc;
  else
    centerc = x->list2c;

  centerv = (t_atom *)getbytes(0);
  centerv = (t_atom *)resizebytes(centerv, 0, centerc * sizeof(t_atom));

  center1v = (t_atom *)getbytes(0);
  center1v = (t_atom *)resizebytes(center1v, 0, centerc * sizeof(t_atom));

  mean = mean1 = std = std1 = corr = 0.0; 
  
  //step1: find the mean, and create lists centered about the mean
  mean = listtool_mean(x, 0, centerc, argv);
  mean1 = listtool_mean(x, 0, centerc, x->list2v);

  for(i=0; i<centerc; i++)
    {
      SETFLOAT(centerv+i, atom_getfloat(argv+i)-mean);
      SETFLOAT(center1v+i, atom_getfloat(x->list2v+i)-mean1);
    }

  //step2:find the standard deviation of those two lists:
  for(i=0; i<centerc; i++)
    {
      std+=(atom_getfloat(centerv+i)*atom_getfloat(centerv+i));
      std1+=(atom_getfloat(center1v+i)*atom_getfloat(center1v+i));
    }
  std = sqrt(std/(centerc-1));
  std1 = sqrt(std1/(centerc-1));

  //step3: calculate the running products and find the corr:
 for(i=0; i<centerc; i++)
   corr += (atom_getfloat(centerv+i)*atom_getfloat(center1v +i));
 
 corr /= (centerc-1);
 corr = corr/(std*std1);

 //ouput and free memory

 outlet_float(x->info, corr);

 freebytes(centerv, centerc * sizeof(t_float));
 freebytes(center1v, centerc * sizeof(t_float));

 zeros:
  outlet_list(x->list, 0, argc, argv);

 over:;
}

static void listtool_table(t_listtool *x, t_symbol *s, int argc, t_atom *argv)
{
  t_garray *a;
  int vecsize = argc-1;
  int i;

  x->x_arrayname = atom_getsymbol(argv);

  if(!(a = (t_garray *)pd_findbyclass(x->x_arrayname, garray_class)))
    pd_error(x, "%s: no such array", s->s_name);
 else if(!garray_getfloatwords(a, &x->arraypts, &x->x_vec))
    	pd_error(x, "%s: bad template for table", x->x_arrayname->s_name);
 else if(x->arraypts < (argc-1))
   pd_error(x, "resize your array, %s to at least %d points", x->x_arrayname->s_name, (argc-1));
  else
    {
      for(i=1; i<argc; i++)
	x->x_vec[i-1].w_float = atom_getfloat(argv+i);
      
      garray_redraw(a);

    }
}

static void listtool_list2(t_listtool *x, t_symbol *s, int argc, t_atom *argv)
{
  int i;
  x->list2c=argc;
  x->list2v = resizebytes(x->list2v, 0, argc*sizeof(t_atom));
  for(i=0; i<argc; i++)
    SETFLOAT(x->list2v+i, atom_getfloat(argv+i));
}

static void *listtool_new(t_symbol *s, int argc, t_atom *argv)
{
    t_listtool *x = (t_listtool *)pd_new(listtool_class);
    x->info = outlet_new(&x->x_obj, &s_float);
    x->list = outlet_new(&x->x_obj, &s_list);
    x->indeces = outlet_new(&x->x_obj, &s_list);
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, gensym("list"), gensym("list2"));
    //setup memory for right inlet list:
    x->list2v=(t_atom *)getbytes(0);
    x->list2c = 0;
    //this is copy/pasted right out of william's code:
    x->rand_state = (int)clock_getlogicaltime(); // seed with (int) logical time
    return (x);
}
static void listtool_free(t_listtool *x)
{
  if(x->list2v)
    freebytes(x->list2v, x->list2c* sizeof(t_atom));
}
void listtool_setup(void)
{
  listtool_class = class_new(gensym("listtool"), 
			     (t_newmethod)listtool_new, 
			     (t_method)listtool_free,
			     sizeof(t_listtool), 
			     CLASS_DEFAULT, A_GIMME, 0);

  class_addmethod(listtool_class,
		  (t_method)listtool_list2,
		  gensym("list2"),
		  A_GIMME,
		  0);
  
  class_addmethod(listtool_class, 
		  (t_method)listtool_normalize, 
		  gensym("normalize"), 
		  A_GIMME,
		  0);

 class_addmethod(listtool_class, 
		  (t_method)listtool_normalize_sum, 
		  gensym("normalize_sum"), 
		  A_GIMME,
		  0);

  class_addmethod(listtool_class, 
		  (t_method)listtool_size, 
		  gensym("size"), 
		  A_GIMME,
		  0);

  class_addmethod(listtool_class, 
		  (t_method)listtool_powtodb, 
		  gensym("powtodb"), 
		  A_GIMME,
		  0);

 class_addmethod(listtool_class, 
		  (t_method)listtool_rmstodb, 
		  gensym("rmstodb"), 
		  A_GIMME,
		  0);

  class_addmethod(listtool_class, 
		  (t_method)listtool_min, 
		  gensym("min"), 
		  A_GIMME,
		  0);

 class_addmethod(listtool_class, 
		  (t_method)listtool_max, 
		  gensym("max"), 
		  A_GIMME,
		  0);

 class_addmethod(listtool_class, 
		  (t_method)listtool_nearest, 
		  gensym("nearest"), 
		  A_GIMME,
		  0);

 class_addmethod(listtool_class, 
		  (t_method)listtool_equals, 
		  gensym("equals"), 
		  A_GIMME,
		  0);

 class_addmethod(listtool_class, 
		  (t_method)listtool_greater, 
		  gensym("greater"), 
		  A_GIMME,
		  0);

 class_addmethod(listtool_class, 
		  (t_method)listtool_greater_equal, 
		  gensym("greater_equal"), 
		  A_GIMME,
		  0);

 class_addmethod(listtool_class, 
		  (t_method)listtool_less, 
		  gensym("less"), 
		  A_GIMME,
		  0);

 class_addmethod(listtool_class, 
		  (t_method)listtool_less_equal, 
		  gensym("less_equal"), 
		  A_GIMME,
		  0);

 class_addmethod(listtool_class, 
		  (t_method)listtool_choose, 
		  gensym("choose"), 
		  A_GIMME,
		  0);

 class_addmethod(listtool_class, 
		  (t_method)listtool_const, 
		  gensym("const"), 
		  A_GIMME,
		  0);

 class_addmethod(listtool_class, 
		  (t_method)listtool_offset, 
		  gensym("offset"), 
		  A_GIMME,
		  0);

 class_addmethod(listtool_class, 
		  (t_method)listtool_shift, 
		  gensym("shift"), 
		  A_GIMME,
		  0);

 class_addmethod(listtool_class, 
		  (t_method)listtool_shift0, 
		  gensym("shift0"), 
		  A_GIMME,
		  0);

 class_addmethod(listtool_class, 
		  (t_method)listtool_scale, 
		  gensym("scale"), 
		  A_GIMME,
		  0);

 class_addmethod(listtool_class, 
		  (t_method)listtool_invert, 
		  gensym("invert"), 
		  A_GIMME,
		  0);

 class_addmethod(listtool_class, 
		  (t_method)listtool_reverse, 
		  gensym("reverse"), 
		  A_GIMME,
		  0);

 class_addmethod(listtool_class, 
		  (t_method)listtool_remove, 
		  gensym("remove"), 
		  A_GIMME,
		  0);

 class_addmethod(listtool_class, 
		  (t_method)listtool_remove0, 
		  gensym("remove0"), 
		  A_GIMME,
		  0);

class_addmethod(listtool_class, 
		  (t_method)listtool_integrate, 
		  gensym("integrate"), 
		  A_GIMME,
		  0);

class_addmethod(listtool_class, 
		  (t_method)listtool_differentiate, 
		  gensym("differentiate"), 
		  A_GIMME,
		  0);

class_addmethod(listtool_class, 
		  (t_method)listtool_add, 
		  gensym("add"), 
		  A_GIMME,
		  0);

class_addmethod(listtool_class, 
		  (t_method)listtool_subtract, 
		  gensym("subtract"), 
		  A_GIMME,
		  0);

class_addmethod(listtool_class, 
		  (t_method)listtool_multiply, 
		  gensym("multiply"), 
		  A_GIMME,
		  0);

class_addmethod(listtool_class, 
		  (t_method)listtool_divide, 
		  gensym("divide"), 
		  A_GIMME,
		  0);

class_addmethod(listtool_class, 
		  (t_method)listtool_sum, 
		  gensym("sum"), 
		  A_GIMME,
		  0);

class_addmethod(listtool_class, 
		  (t_method)listtool_mean, 
		  gensym("mean"), 
		  A_GIMME,
		  0);

class_addmethod(listtool_class, 
		  (t_method)listtool_product, 
		  gensym("product"), 
		  A_GIMME,
		  0);

class_addmethod(listtool_class, 
		  (t_method)listtool_geomean, 
		  gensym("geomean"), 
		  A_GIMME,
		  0);

class_addmethod(listtool_class, 
		  (t_method)listtool_std, 
		  gensym("std"), 
		  A_GIMME,
		  0);

class_addmethod(listtool_class, 
		  (t_method)listtool_dot, 
		  gensym("dot"), 
		  A_GIMME,
		  0);

class_addmethod(listtool_class, 
		  (t_method)listtool_euclid, 
		  gensym("euclid"), 
		  A_GIMME,
		  0);

class_addmethod(listtool_class, 
		  (t_method)listtool_taxi, 
		  gensym("taxi"), 
		  A_GIMME,
		  0);

class_addmethod(listtool_class, 
		  (t_method)listtool_corr, 
		  gensym("corr"), 
		  A_GIMME,
		  0);

 class_addmethod(listtool_class,
 		  (t_method)listtool_table,
 		  gensym("table"),
 		  A_GIMME,
 		  0);
 
}
