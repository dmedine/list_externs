/* listorder is a simple list manipulation utility for Pd */
/* Copyright David Medine */
/* released under the terms of the GPL */


#include "m_pd.h"
#include <stdio.h>
/* -------------------------- listorder ------------------------------ */
static t_class *listorder_class;

typedef struct _listorder
{
  t_object x_obj;
  t_float flag;
  t_float step;
} t_listorder;


static void listorder_flag(t_listorder *x, t_float f)
{
  x->flag = f;
  if(x->flag!=0)
    x->flag = 1;
}

static void listorder_step(t_listorder *x, t_float f)
{
  x->step = (int)f;
  if(x->step==0)x->step = 1;
}

static void listorder_sort(t_listorder *x, t_symbol *s, int argc, t_atom *argv)
{
  t_atom *outv;
  t_float *temp;
  int outc = argc;
  int step = (int)x->step;
  int z, y, t; 
  t_float holder[step+1];
  
  outv = (t_atom *)getbytes(0);
  outv = (t_atom *)resizebytes(outv, 0, outc * sizeof(t_atom));
  temp = (t_float *)getbytes(0);
  temp = (t_float *)resizebytes(temp, 0, outc * sizeof(t_float));
  
  for(z = 0; z<outc; z++)
    temp[z] = atom_getfloat(argv+z);

  //bubble sort algorithm
  if(x->flag==1)//forwards
    {
      for(z = 0; z < outc; z+=step)
	{
	  for(y = 0; y < outc-(step); y+=step)
	    if(temp[y] > temp[y+step])
	      {
		for(t=0; t<step; t++)
		  {
		    holder[t] = temp[y+step+t];
		    temp[y+step+t] = temp[y+t];
		    temp[y+t] = holder[t];
		  }
	      }
	}
    }

  else//backwards
    {
      for(z = 0; z < outc; z++)
	{
	  for(y = 0; y < outc-1; y++)
	    if(temp[y] < temp[y+1])
	      {
		for(t=0; t<step; t++)
		  {
		    holder[t] = temp[y+step+t];
		    temp[y+step+t] = temp[y+t];
		    temp[y+t] = holder[t];
		  }
	      }
	}
    }

  for(z = 0; z<outc; z++)
    SETFLOAT(outv+z, temp[z]);

  outlet_list(x->x_obj.ob_outlet, &s_list, outc, outv);

  freebytes(temp, outc * sizeof(t_float));
  freebytes(outv, outc * sizeof(t_atom));

}


static void *listorder_new(t_symbol *s, int argc, t_atom *argv)
{
    t_listorder *x = (t_listorder *)pd_new(listorder_class);
    outlet_new(&x->x_obj, &s_list);
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, gensym("float"), gensym("flag"));

     x->flag = 1;
     x->step = 1;

    return (x);
}



void listorder_setup(void)
{
  listorder_class = class_new(gensym("listorder"), 
			      (t_newmethod)listorder_new, 
			      0,
			      sizeof(t_listorder), 
			      CLASS_DEFAULT, 
			      A_GIMME, 
			      0);

  class_addmethod(listorder_class, 
		  (t_method)listorder_flag,
		  gensym("flag"), 
		  A_FLOAT, 
		  0);

  class_addlist(listorder_class, listorder_sort);

  class_sethelpsymbol(listorder_class, gensym("help-listorder")); 

  class_addmethod(listorder_class, 
		  (t_method)listorder_step, 
		  gensym("step"), 
		  A_DEFFLOAT, 
		  0);
}
