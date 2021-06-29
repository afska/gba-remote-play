/* GSM decoder tuned for GBA
 * Copyright 2004 by Damian Yerrick.
 * Based on GSM RPE-LTP 1.0.10, Copyright 1992-1994 by Jutta Degener
 * and Carsten Bormann, Technische Universitaet Berlin.
 * See the accompanying file "TOAST-COPYRIGHT.txt"
 * for details.  THERE IS ABSOLUTELY NO WARRANTY FOR THIS SOFTWARE.
 */

/*
 *  See private.h for the more commonly used macro versions.
 */

#include <stdio.h>
#include <stdlib.h>
/* #include <string.h> */
#define assert(x) ((void)0)

#include "config.h"
#include "gsm.h"
#include "private.h"
#include "proto.h"

#if 0 /* turn this ON to enable color profiling */
#define PROFILE_COLOR(r, g, b) \
  (*(volatile unsigned short*)0x05000000 = (r) | (g) << 5 | (b) << 10)
#else
#define PROFILE_COLOR(r, g, b) ((void)0)
#endif

/* begin add.h ********************/

#define saturate(x) \
  ((x) < MIN_WORD ? MIN_WORD : (x) > MAX_WORD ? MAX_WORD : (x))

#if 0
word gsm_sub P2((a,b), word a, word b)
{
  longword diff = (longword)a - (longword)b;
  return saturate(diff);
}
#endif

__attribute__((section(".iwram"))) static word gsm_asr P2((a, n),
                                                          word a,
                                                          int n) {
  if (n >= 16)
    return -(a < 0);
  if (n <= -16)
    return 0;
  if (n < 0)
    return a << -n;

#ifdef SASR
  return a >> n;
#else
  if (a >= 0)
    return a >> n;
  else
    return -(word)(-(uword)a >> n);
#endif
}

__attribute__((section(".iwram"))) static word gsm_asl P2((a, n),
                                                          word a,
                                                          int n) {
  if (n >= 16)
    return 0;
  if (n <= -16)
    return -(a < 0);
  if (n < 0)
    return gsm_asr(a, -n);
  return a << n;
}

/* begin long_term.c ********************/

/*
 *  4.2.11 .. 4.2.12 LONG TERM PREDICTOR (LTP) SECTION
 */

/* mod by Damian Yerrick: remove all analysis code */

/* 4.3.2 */

/* GSM_LTSF() is the second biggest bottleneck next to GSM_STSF().
   Some unrolling here helped quite a bit.
*/
__attribute__((section(".iwram"))) static void Gsm_Long_Term_Synthesis_Filtering
P5((S, Ncr, bcr, erp, drp),
   struct gsm_state* S,

   word Ncr,
   word bcr,
   word* erp, /* [0..39]		  	 IN */
   word* drp  /* [-120..-1] IN, [-120..40] OUT */
   )
/*
 *  This procedure uses the bcr and Ncr parameter to realize the
 *  long term synthesis filtering.  The decoding of bcr needs
 *  table 4.3b.
 */
{
  int k, brp, drpp, Nr;

  /*  Check the limits of Nr.
   */
  Nr = (Ncr < 40 || Ncr > 120) ? S->nrp : Ncr;
  S->nrp = Nr;
  assert(Nr >= 40 && Nr <= 120);

  /*  Decoding of the LTP gain bcr
   */
  brp = gsm_QLB[bcr];

  /*  Computation of the reconstructed short term residual
   *  signal drp[0..39]
   */
  assert(brp != MIN_WORD);

  for (k = 0; k <= 39;) {
#undef LTSF_STEP
#define LTSF_STEP                      \
  drpp = GSM_MULT_R(brp, drp[k - Nr]); \
  drp[k] = erp[k] + drpp;              \
  k++;

    LTSF_STEP
    LTSF_STEP
    LTSF_STEP
    LTSF_STEP
    LTSF_STEP
  }

  /*
   *  Update of the reconstructed short term residual signal
   *  drp[ -1..-120 ]
   */

  for (k = 0; k <= 119; k += 5) {
    drp[-120 + k] = drp[-80 + k];
    drp[-119 + k] = drp[-79 + k];
    drp[-118 + k] = drp[-78 + k];
    drp[-117 + k] = drp[-77 + k];
    drp[-116 + k] = drp[-76 + k];
  }
}

/* begin short_term.c ********************/

/*
 *  SHORT TERM ANALYSIS FILTERING SECTION
 */

/* 4.2.8 */

__attribute__((
    section(".iwram"))) static void Decoding_of_the_coded_Log_Area_Ratios
P2((LARc, LARpp),
   word* LARc,  /* coded log area ratio	[0..7] 	IN	*/
   word* LARpp) /* out: decoded ..			*/
{
  register word temp1 /* , temp2 */;
  register long ltmp; /* for GSM_ADD */

  /*  This procedure requires for efficient implementation
   *  two tables.
   *
   *  INVA[1..8] = integer( (32768 * 8) / real_A[1..8])
   *  MIC[1..8]  = minimum value of the LARc[1..8]
   */

  /*  Compute the LARpp[1..8]
   */

  /* 	for (i = 1; i <= 8; i++, B++, MIC++, INVA++, LARc++, LARpp++) {
   *
   *		temp1  = GSM_ADD( *LARc, *MIC ) << 10;
   *		temp2  = *B << 1;
   *		temp1  = GSM_SUB( temp1, temp2 );
   *
   *		assert(*INVA != MIN_WORD);
   *
   *		temp1  = GSM_MULT_R( *INVA, temp1 );
   *		*LARpp = GSM_ADD( temp1, temp1 );
   *	}
   */

#undef STEP
#define STEP(B, MIC, INVA)             \
  temp1 = GSM_ADD(*LARc++, MIC) << 10; \
  temp1 = GSM_SUB(temp1, B << 1);      \
  temp1 = GSM_MULT_R(INVA, temp1);     \
  *LARpp++ = GSM_ADD(temp1, temp1);

  STEP(0, -32, 13107);
  STEP(0, -32, 13107);
  STEP(2048, -16, 13107);
  STEP(-2560, -16, 13107);

  STEP(94, -8, 19223);
  STEP(-1792, -8, 17476);
  STEP(-341, -4, 31454);
  STEP(-1144, -4, 29708);

  /* NOTE: the addition of *MIC is used to restore
   * 	 the sign of *LARc.
   */
}

/* 4.2.9 */
/* Computation of the quantized reflection coefficients
 */

/* 4.2.9.1  Interpolation of the LARpp[1..8] to get the LARp[1..8]
 */

/*
 *  Within each frame of 160 analyzed speech samples the short term
 *  analysis and synthesis filters operate with four different sets of
 *  coefficients, derived from the previous set of decoded LARs(LARpp(j-1))
 *  and the actual set of decoded LARs (LARpp(j))
 *
 * (Initial value: LARpp(j-1)[1..8] = 0.)
 */

__attribute__((section(".iwram"))) static void Coefficients_0_12
P3((LARpp_j_1, LARpp_j, LARp), word* LARpp_j_1, word* LARpp_j, word* LARp) {
  int i;
  longword ltmp;

  for (i = 1; i <= 8; i++, LARp++, LARpp_j_1++, LARpp_j++) {
    *LARp = GSM_ADD(SASR(*LARpp_j_1, 2), SASR(*LARpp_j, 2));
    *LARp = GSM_ADD(*LARp, SASR(*LARpp_j_1, 1));
  }
}

__attribute__((section(".iwram"))) static void Coefficients_13_26
P3((LARpp_j_1, LARpp_j, LARp), word* LARpp_j_1, word* LARpp_j, word* LARp) {
  int i;
  longword ltmp;
  for (i = 1; i <= 8; i++, LARpp_j_1++, LARpp_j++, LARp++) {
    *LARp = GSM_ADD(SASR(*LARpp_j_1, 1), SASR(*LARpp_j, 1));
  }
}

__attribute__((section(".iwram"))) static void Coefficients_27_39
P3((LARpp_j_1, LARpp_j, LARp), word* LARpp_j_1, word* LARpp_j, word* LARp) {
  int i;
  longword ltmp;

  for (i = 1; i <= 8; i++, LARpp_j_1++, LARpp_j++, LARp++) {
    *LARp = GSM_ADD(SASR(*LARpp_j_1, 2), SASR(*LARpp_j, 2));
    *LARp = GSM_ADD(*LARp, SASR(*LARpp_j, 1));
  }
}

__attribute__((section(".iwram"))) static void Coefficients_40_159
P2((LARpp_j, LARp), word* LARpp_j, word* LARp) {
  int i;

  for (i = 1; i <= 8; i++, LARp++, LARpp_j++)
    *LARp = *LARpp_j;
}

/* 4.2.9.2 */

__attribute__((section(".iwram"))) static void LARp_to_rp
P1((LARp), word* LARp) /* [0..7] IN/OUT  */
                       /*
                        *  The input of this procedure is the interpolated LARp[0..7] array.
                        *  The reflection coefficients, rp[i], are used in the analysis
                        *  filter and in the synthesis filter.
                        */
{
  int i;
  word temp;
  longword ltmp;

  for (i = 1; i <= 8; i++, LARp++) {
    /* temp = GSM_ABS( *LARp );
     *
     * if (temp < 11059) temp <<= 1;
     * else if (temp < 20070) temp += 11059;
     * else temp = GSM_ADD( temp >> 2, 26112 );
     *
     * *LARp = *LARp < 0 ? -temp : temp;
     */

    if (*LARp < 0) {
      temp = *LARp == MIN_WORD ? MAX_WORD : -(*LARp);
      *LARp = -((temp < 11059) ? temp << 1
                               : ((temp < 20070) ? temp + 11059
                                                 : GSM_ADD(temp >> 2, 26112)));
    } else {
      temp = *LARp;
      *LARp = (temp < 11059)
                  ? temp << 1
                  : ((temp < 20070) ? temp + 11059 : GSM_ADD(temp >> 2, 26112));
    }
  }
}

/* DY writes:
   This is bottleneck one.
   By simplifying and then unrolling the inner loop here, I've
   cut a LOT of CPU time off the function since the first public
   release of the decoder.  Now on to the other bottleneck:
   Gsm_Long_Term_Synthesis_Filtering */

__attribute__((section(".iwram"))) static void Short_term_synthesis_filtering
P5((S, rrp, k, wt, sr),
   struct gsm_state* S,
   word* rrp, /* [0..7]	IN	*/
   int k,     /* k_end - k_start	*/
   word* wt,  /* [0..k-1]	IN	*/
   word* sr   /* [0..k-1]	OUT	*/
) {
  word* v = S->v;

  PROFILE_COLOR(31, 31, 0);
  while (k--) {
    int sri = *wt++;
    int rrp_i, v_i;

    /* Note to any other developer:
       THIS is the readable way to unroll a loop. */

#undef STSF_STEP
#define STSF_STEP(i)                 \
  rrp_i = rrp[i];                    \
  v_i = v[i];                        \
  sri -= GSM_MULT_R((rrp_i), (v_i)); \
  v[i + 1] = v_i + GSM_MULT_R((rrp_i), (sri));

    STSF_STEP(7)
    STSF_STEP(6)
    STSF_STEP(5)
    STSF_STEP(4)
    STSF_STEP(3)
    STSF_STEP(2)
    STSF_STEP(1)
    STSF_STEP(0)

    *sr++ = v[0] = sri;
  }
  PROFILE_COLOR(0, 0, 31);
}

__attribute__((section(".iwram"))) static void Gsm_Short_Term_Synthesis_Filter
P4((S, LARcr, wt, s),
   struct gsm_state* S,

   word* LARcr, /* received log area ratios [0..7] IN  */
   word* wt,    /* received d [0..159]		   IN  */

   word* s /* signal   s [0..159]		  OUT  */
) {
  word* LARpp_j = S->LARpp[S->j];
  word* LARpp_j_1 = S->LARpp[S->j ^= 1];

  word LARp[8];

#undef FILTER
#define FILTER Short_term_synthesis_filtering

  PROFILE_COLOR(31, 0, 15);
  Decoding_of_the_coded_Log_Area_Ratios(LARcr, LARpp_j);
  PROFILE_COLOR(0, 0, 31);

  Coefficients_0_12(LARpp_j_1, LARpp_j, LARp);
  LARp_to_rp(LARp);
  FILTER(S, LARp, 13, wt, s);

  Coefficients_13_26(LARpp_j_1, LARpp_j, LARp);
  LARp_to_rp(LARp);
  FILTER(S, LARp, 14, wt + 13, s + 13);

  Coefficients_27_39(LARpp_j_1, LARpp_j, LARp);
  LARp_to_rp(LARp);
  FILTER(S, LARp, 13, wt + 27, s + 27);

  Coefficients_40_159(LARpp_j, LARp);
  LARp_to_rp(LARp);
  FILTER(S, LARp, 120, wt + 40, s + 40);
}

/* begin lpc.c ********************/

/* 4.12.15 */

__attribute__((
    section(".iwram"))) static void APCM_quantization_xmaxc_to_exp_mant
P3((xmaxc, exp_out, mant_out),
   word xmaxc,     /* IN 	*/
   word* exp_out,  /* OUT	*/
   word* mant_out) /* OUT  */
{
  word exp, mant;

  /* Compute exponent and mantissa of the decoded version of xmaxc
   */

  exp = 0;
  if (xmaxc > 15)
    exp = SASR(xmaxc, 3) - 1;
  mant = xmaxc - (exp << 3);

  if (mant == 0) {
    exp = -4;
    mant = 7;
  } else {
    while (mant <= 7) {
      mant = mant << 1 | 1;
      exp--;
    }
    mant -= 8;
  }

  assert(exp >= -4 && exp <= 6);
  assert(mant >= 0 && mant <= 7);

  *exp_out = exp;
  *mant_out = mant;
}

/* 4.2.16 */

__attribute__((section(".iwram"))) static void APCM_inverse_quantization
P4((xMc, mant, exp, xMp),
   register word* xMc, /* [0..12]			IN 	*/
   word mant,
   word exp,
   register word* xMp) /* [0..12]			OUT 	*/
                       /*
                        *  This part is for decoding the RPE sequence of coded xMc[0..12]
                        *  samples to obtain the xMp[0..12] array.  Table 4.6 is used to get
                        *  the mantissa of xmaxc (FAC[0..7]).
                        */
{
  int i;
  word temp, temp1, temp2, temp3;
  longword ltmp;

  assert(mant >= 0 && mant <= 7);

  temp1 = gsm_FAC[mant];   /* see 4.2-15 for mant */
  temp2 = GSM_SUB(6, exp); /* see 4.2-15 for exp  */
  temp3 = gsm_asl(1, GSM_SUB(temp2, 1));

  for (i = 13; i--;) {
    assert(*xMc <= 7 && *xMc >= 0); /* 3 bit unsigned */

    /* temp = gsm_sub( *xMc++ << 1, 7 ); */
    temp = (*xMc++ << 1) - 7;        /* restore sign   */
    assert(temp <= 7 && temp >= -7); /* 4 bit signed   */

    temp <<= 12; /* 16 bit signed  */
    temp = GSM_MULT_R(temp1, temp);
    temp = GSM_ADD(temp, temp3);
    /* *xMp++ = gsm_asr( temp, temp2 ); */
    *xMp++ = temp >> temp2;
  }
}

/* 4.2.17 */

__attribute__((section(".iwram"))) static void RPE_grid_positioning
P3((Mc, xMp, ep),
   word Mc,            /* grid position	IN	*/
   register word* xMp, /* [0..12]		IN	*/
   register word* ep   /* [0..39]		OUT	*/
   )
/*
 *  This procedure computes the reconstructed long term residual signal
 *  ep[0..39] for the LTP analysis filter.  The inputs are the Mc
 *  which is the grid position selection and the xMp[0..12] decoded
 *  RPE samples which are upsampled by a factor of 3 by inserting zero
 *  values.
 */
{
  int i;

  assert(0 <= Mc && Mc <= 3);

  /* FIXME: Get rid of this Duff's device */

#if 0 /* turn on 1 if you're on a machine where Duff's device is fast */
  i = 13;
  switch (Mc) {
  case 3: *ep++ = 0;
  case 2:  do {
    *ep++ = 0;
  case 1:         *ep++ = 0;
  case 0:         *ep++ = *xMp++;
  } while (--i);
  }
#else
  switch (Mc) {
    case 3:
      *ep++ = 0;
    case 2:
      *ep++ = 0;
    case 1:
      *ep++ = 0;
    case 0:
      *ep++ = *xMp++;
  }
  i = 12;
  do {
    *ep++ = 0;
    *ep++ = 0;
    *ep++ = *xMp++;
  } while (--i);

#endif
  while (++Mc < 4)
    *ep++ = 0;

  /*

  int i, k;
  for (k = 0; k <= 39; k++) ep[k] = 0;
  for (i = 0; i <= 12; i++) {
  ep[ Mc + (3*i) ] = xMp[i];
  }
  */
}

/* 4.2.18 */

__attribute__((section(".iwram"))) static void Gsm_RPE_Decoding P5(
    (S, xmaxcr, Mcr, xMcr, erp),
    struct gsm_state* S,

    word xmaxcr,
    word Mcr,
    word* xMcr, /* [0..12], 3 bits 		IN	*/
    word* erp   /* [0..39]			OUT 	*/
) {
  word exp, mant;
  word xMp[13];

  APCM_quantization_xmaxc_to_exp_mant(xmaxcr, &exp, &mant);
  APCM_inverse_quantization(xMcr, mant, exp, xMp);
  RPE_grid_positioning(Mcr, xMp, erp);
}

/* begin decode.c ********************/

/*
 *  4.3 FIXED POINT IMPLEMENTATION OF THE RPE-LTP DECODER
 */

__attribute__((section(".iwram"))) static void Postprocessing
P2((S, s), struct gsm_state* S, register word* s) {
  register int k;
  register word msr = S->msr;
  register longword ltmp; /* for GSM_ADD */
  register word tmp;

  for (k = 160; k--; s++) {
    tmp = GSM_MULT_R(msr, 28180);
    msr = GSM_ADD(*s, tmp);          /* Deemphasis 	     */
    *s = GSM_ADD(msr, msr) & 0xFFF8; /* Truncation & Upscaling */
  }
  S->msr = msr;
  PROFILE_COLOR(29, 31, 27);
}

__attribute__((section(".iwram"))) static void Gsm_Decoder
P8((S, LARcr, Ncr, bcr, Mcr, xmaxcr, xMcr, s),
   struct gsm_state* S,

   word* LARcr, /* [0..7]		IN	*/

   word* Ncr,    /* [0..3] 		IN 	*/
   word* bcr,    /* [0..3]		IN	*/
   word* Mcr,    /* [0..3] 		IN 	*/
   word* xmaxcr, /* [0..3]		IN 	*/
   word* xMcr,   /* [0..13*4]		IN	*/

   word* s) /* [0..159]		OUT 	*/

{
  int j, k;
  word erp[40], wt[160];
  word* drp = S->dp0 + 120;

  for (j = 0; j <= 3; j++, xmaxcr++, bcr++, Ncr++, Mcr++, xMcr += 13) {
    PROFILE_COLOR(31, 23, 0);
    Gsm_RPE_Decoding(S, *xmaxcr, *Mcr, xMcr, erp);
    PROFILE_COLOR(0, 23, 31);
    Gsm_Long_Term_Synthesis_Filtering(S, *Ncr, *bcr, erp, drp);
    PROFILE_COLOR(0, 23, 23);
    {
      word* wt_cur = wt + j * 40;
      for (k = 0; k <= 39; k++)
        *wt_cur++ = drp[k];
    }
  }

  /* Goes hot pink then yellow here */

  /* vba seems to think gsm spends most of its time in Gsm_STSF */
  Gsm_Short_Term_Synthesis_Filter(S, LARcr, wt, s);
  Postprocessing(S, s);
}

#if 0

/* begin gsm_create.c ********************/

gsm gsm_create P0()
{
  gsm  r;

  r = (gsm)malloc(sizeof(struct gsm_state));
  if (!r) return r;

  memset((char *)r, 0, sizeof(*r));
  r->nrp = 40;

  return r;
}

#endif

/* begin gsm_decode.c ********************/

__attribute__((section(".iwram"), long_call)) int gsm_decode
P3((s, c, target), gsm s, const gsm_byte* c, gsm_signal* target) {
  word LARc[8], Nc[4], Mc[4], bc[4], xmaxc[4], xmc[13 * 4];

  if (((*c >> 4) & 0x0F) != GSM_MAGIC)
    return -1;

  PROFILE_COLOR(31, 0, 0);

  LARc[0] = (*c++ & 0xF) << 2; /* 1 */
  LARc[0] |= (*c >> 6) & 0x3;
  LARc[1] = *c++ & 0x3F;
  LARc[2] = (*c >> 3) & 0x1F;
  LARc[3] = (*c++ & 0x7) << 2;
  LARc[3] |= (*c >> 6) & 0x3;
  LARc[4] = (*c >> 2) & 0xF;
  LARc[5] = (*c++ & 0x3) << 2;
  LARc[5] |= (*c >> 6) & 0x3;
  LARc[6] = (*c >> 3) & 0x7;
  LARc[7] = *c++ & 0x7;
  Nc[0] = (*c >> 1) & 0x7F;
  bc[0] = (*c++ & 0x1) << 1;
  bc[0] |= (*c >> 7) & 0x1;
  Mc[0] = (*c >> 5) & 0x3;
  xmaxc[0] = (*c++ & 0x1F) << 1;
  xmaxc[0] |= (*c >> 7) & 0x1;
  xmc[0] = (*c >> 4) & 0x7;
  xmc[1] = (*c >> 1) & 0x7;
  xmc[2] = (*c++ & 0x1) << 2;
  xmc[2] |= (*c >> 6) & 0x3;
  xmc[3] = (*c >> 3) & 0x7;
  xmc[4] = *c++ & 0x7;
  xmc[5] = (*c >> 5) & 0x7;
  xmc[6] = (*c >> 2) & 0x7;
  xmc[7] = (*c++ & 0x3) << 1; /* 10 */
  xmc[7] |= (*c >> 7) & 0x1;
  xmc[8] = (*c >> 4) & 0x7;
  xmc[9] = (*c >> 1) & 0x7;
  xmc[10] = (*c++ & 0x1) << 2;
  xmc[10] |= (*c >> 6) & 0x3;
  xmc[11] = (*c >> 3) & 0x7;
  xmc[12] = *c++ & 0x7;
  Nc[1] = (*c >> 1) & 0x7F;
  bc[1] = (*c++ & 0x1) << 1;
  bc[1] |= (*c >> 7) & 0x1;
  Mc[1] = (*c >> 5) & 0x3;
  xmaxc[1] = (*c++ & 0x1F) << 1;
  xmaxc[1] |= (*c >> 7) & 0x1;
  xmc[13] = (*c >> 4) & 0x7;
  xmc[14] = (*c >> 1) & 0x7;
  xmc[15] = (*c++ & 0x1) << 2;
  xmc[15] |= (*c >> 6) & 0x3;
  xmc[16] = (*c >> 3) & 0x7;
  xmc[17] = *c++ & 0x7;
  xmc[18] = (*c >> 5) & 0x7;
  xmc[19] = (*c >> 2) & 0x7;
  xmc[20] = (*c++ & 0x3) << 1;
  xmc[20] |= (*c >> 7) & 0x1;
  xmc[21] = (*c >> 4) & 0x7;
  xmc[22] = (*c >> 1) & 0x7;
  xmc[23] = (*c++ & 0x1) << 2;
  xmc[23] |= (*c >> 6) & 0x3;
  xmc[24] = (*c >> 3) & 0x7;
  xmc[25] = *c++ & 0x7;
  Nc[2] = (*c >> 1) & 0x7F;
  bc[2] = (*c++ & 0x1) << 1; /* 20 */
  bc[2] |= (*c >> 7) & 0x1;
  Mc[2] = (*c >> 5) & 0x3;
  xmaxc[2] = (*c++ & 0x1F) << 1;
  xmaxc[2] |= (*c >> 7) & 0x1;
  xmc[26] = (*c >> 4) & 0x7;
  xmc[27] = (*c >> 1) & 0x7;
  xmc[28] = (*c++ & 0x1) << 2;
  xmc[28] |= (*c >> 6) & 0x3;
  xmc[29] = (*c >> 3) & 0x7;
  xmc[30] = *c++ & 0x7;
  xmc[31] = (*c >> 5) & 0x7;
  xmc[32] = (*c >> 2) & 0x7;
  xmc[33] = (*c++ & 0x3) << 1;
  xmc[33] |= (*c >> 7) & 0x1;
  xmc[34] = (*c >> 4) & 0x7;
  xmc[35] = (*c >> 1) & 0x7;
  xmc[36] = (*c++ & 0x1) << 2;
  xmc[36] |= (*c >> 6) & 0x3;
  xmc[37] = (*c >> 3) & 0x7;
  xmc[38] = *c++ & 0x7;
  Nc[3] = (*c >> 1) & 0x7F;
  bc[3] = (*c++ & 0x1) << 1;
  bc[3] |= (*c >> 7) & 0x1;
  Mc[3] = (*c >> 5) & 0x3;
  xmaxc[3] = (*c++ & 0x1F) << 1;
  xmaxc[3] |= (*c >> 7) & 0x1;
  xmc[39] = (*c >> 4) & 0x7;
  xmc[40] = (*c >> 1) & 0x7;
  xmc[41] = (*c++ & 0x1) << 2;
  xmc[41] |= (*c >> 6) & 0x3;
  xmc[42] = (*c >> 3) & 0x7;
  xmc[43] = *c++ & 0x7; /* 30  */
  xmc[44] = (*c >> 5) & 0x7;
  xmc[45] = (*c >> 2) & 0x7;
  xmc[46] = (*c++ & 0x3) << 1;
  xmc[46] |= (*c >> 7) & 0x1;
  xmc[47] = (*c >> 4) & 0x7;
  xmc[48] = (*c >> 1) & 0x7;
  xmc[49] = (*c++ & 0x1) << 2;
  xmc[49] |= (*c >> 6) & 0x3;
  xmc[50] = (*c >> 3) & 0x7;
  xmc[51] = *c & 0x7; /* 33 */

  Gsm_Decoder(s, LARc, Nc, bc, Mc, xmaxc, xmc, target);

  return 0;
}

#if 0

/* begin gsm_destroy.c ********************/

void gsm_destroy P1((S), gsm S)
{
  if (S) free((char *)S);
}

#endif

/* begin table.c ********************/

/* $Header: /tmp_amd/presto/export/kbs/jutta/src/gsm/RCS/table.c,v 1.1
 * 1992/10/28 00:15:50 jutta Exp $ */

/*  Most of these tables are inlined at their point of use.
 */

/*  4.4 TABLES USED IN THE FIXED POINT IMPLEMENTATION OF THE RPE-LTP
 *      CODER AND DECODER
 *
 *	(Most of them inlined, so watch out.)
 */

#define GSM_TABLE_C

/*  Table 4.1  Quantization of the Log.-Area Ratios
 */
/* i 		     1      2      3        4      5      6        7       8 */
word gsm_A[8] = {20480, 20480, 20480, 20480, 13964, 15360, 8534, 9036};
word gsm_B[8] = {0, 0, 2048, -2560, 94, -1792, -341, -1144};
word gsm_MIC[8] = {-32, -32, -16, -16, -8, -8, -4, -4};
word gsm_MAC[8] = {31, 31, 15, 15, 7, 7, 3, 3};

/*  Table 4.2  Tabulation  of 1/A[1..8]
 */
word gsm_INVA[8] = {13107, 13107, 13107, 13107, 19223, 17476, 31454, 29708};

/*   Table 4.3a  Decision level of the LTP gain quantizer
 */
/*  bc		      0	        1	  2	     3			*/
word gsm_DLB[4] = {6554, 16384, 26214, 32767};

/*   Table 4.3b   Quantization levels of the LTP gain quantizer
 */
/* bc		      0          1        2          3			*/
word gsm_QLB[4] = {3277, 11469, 21299, 32767};

/*   Table 4.4	 Coefficients of the weighting filter
 */
/* i		    0      1   2    3   4      5      6     7   8   9    10  */
word gsm_H[11] = {-134, -374, 0, 2054, 5741, 8192, 5741, 2054, 0, -374, -134};

/*   Table 4.5 	 Normalized inverse mantissa used to compute xM/xmax
 */
/* i		 	0        1    2      3      4      5     6      7   */
word gsm_NRFAC[8] = {29128, 26215, 23832, 21846, 20165, 18725, 17476, 16384};

/*   Table 4.6	 Normalized direct mantissa used to compute xM/xmax
 */
/* i                  0      1       2      3      4      5      6      7   */
word gsm_FAC[8] = {18431, 20479, 22527, 24575, 26623, 28671, 30719, 32767};
