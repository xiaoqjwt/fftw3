/*
 * Copyright (c) 2002 Matteo Frigo
 * Copyright (c) 2002 Steven G. Johnson
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

/* $Id: problem.c,v 1.4 2002-07-22 11:27:06 athena Exp $ */

#include "rdft.h"

static void destroy(problem *ego_)
{
     problem_rdft *ego = (problem_rdft *) ego_;
     X(tensor_destroy)(ego->vecsz);
     X(tensor_destroy)(ego->sz);
     X(free)(ego_);
}

static unsigned int hash(const problem *p_)
{
     const problem_rdft *p = (const problem_rdft *) p_;
     return (0xDEADBEEF
	     ^ ((p->I == p->O) * 31)
	     ^ (p->kind * 37)
	     ^ (X(tensor_hash)(p->sz) * 10487)
             ^ (X(tensor_hash)(p->vecsz) * 27197)
	  );
}

static int equal(const problem *ego_, const problem *problem_)
{
     if (ego_->adt == problem_->adt) {
          const problem_rdft *e = (const problem_rdft *) ego_;
          const problem_rdft *p = (const problem_rdft *) problem_;

          return (1

		  /* both in-place or both out-of-place */
                  && ((p->I == p->O) == (e->I == e->O))

		  && p->kind == e->kind

		  && X(tensor_equal)(p->sz, e->sz)
                  && X(tensor_equal)(p->vecsz, e->vecsz)
	       );
     }
     return 0;
}

static void zerotens(tensor sz, R *I)
{
     if (sz.rnk == RNK_MINFTY)
          return;
     else if (sz.rnk == 0)
          I[0] = 0.0;
     else if (sz.rnk == 1) {
          /* this case is redundant but faster */
          uint i, n = sz.dims[0].n;
          int is = sz.dims[0].is;

          for (i = 0; i < n; ++i)
               I[i * is] = 0.0;
     } else if (sz.rnk > 0) {
          uint i, n = sz.dims[0].n;
          int is = sz.dims[0].is;

          sz.dims++;
          sz.rnk--;
          for (i = 0; i < n; ++i)
               zerotens(sz, I + i * is);
     }
}

const char *X(rdft_kind_str)(rdft_kind kind)
{
     static const char s[][8] = { "r2hc", "hc2r", "r2hcII", "hc2rIII" };
     switch (kind) {
	 case R2HC:
	      return s[0];
	 case HC2R:
	      return s[1];
	 case R2HCII:
	      return s[2];
	 case HC2RIII:
	      return s[3];
     }
     A(0);
     return 0;
}

static void print(problem *ego_, printer *p)
{
     const problem_rdft *ego = (const problem_rdft *) ego_;
     p->print(p, "(rdft %d %s %t %t)", 
	      ego->I == ego->O, 
	      X(rdft_kind_str)(ego->kind),
	      &ego->sz,
	      &ego->vecsz);
}

static void zero(const problem *ego_)
{
     const problem_rdft *ego = (const problem_rdft *) ego_;
     tensor sz = X(tensor_append)(ego->vecsz, ego->sz);
     zerotens(sz, ego->I);
     X(tensor_destroy)(sz);
}

static const problem_adt padt =
{
     equal,
     hash,
     zero,
     print,
     destroy
};

int X(problem_rdft_p)(const problem *p)
{
     return (p->adt == &padt);
}

problem *X(mkproblem_rdft)(const tensor sz, const tensor vecsz,
			   R *I, R *O, rdft_kind kind)
{
     problem_rdft *ego =
          (problem_rdft *)X(mkproblem)(sizeof(problem_rdft), &padt);

     ego->sz = X(tensor_compress)(sz);
     ego->vecsz = X(tensor_compress_contiguous)(vecsz);
     ego->I = I;
     ego->O = O;
     ego->kind = kind;

     A(FINITE_RNK(ego->sz.rnk));
     return &(ego->super);
}

/* Same as X(mkproblem_rdft), but also destroy input tensors. */
problem *X(mkproblem_rdft_d)(tensor sz, tensor vecsz,
			     R *I, R *O, rdft_kind kind)
{
     problem *p;
     p = X(mkproblem_rdft)(sz, vecsz, I, O, kind);
     X(tensor_destroy)(vecsz);
     X(tensor_destroy)(sz);
     return p;
}
