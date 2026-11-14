/* R5 diagnostic-only ring. Included after r20w's private state declarations.
 * No traps, counters or production policy are changed. All calls run under BKL. */
#define R5_TRACE_ROWS 512u
#define R5_TRACE_WORDS 12u
unsigned int r5_cache_trace[R5_TRACE_ROWS][R5_TRACE_WORDS];
unsigned int r5_cache_trace_count = 0, r5_cache_trace_frozen = 0;
static unsigned char r5_unreported[R20C_MAX_ENT];
static unsigned int r5_read_seq[JOB_MAX], r5_last_error[JOB_MAX];
static unsigned char r5_read_active[JOB_MAX];
static void r5_trace(unsigned int kind, unsigned int e, unsigned int extra)
{
 unsigned int *p; int j = job_current;
 if (e < nent) {
  if (kind == 1u) r5_unreported[e] = 1;
  if (kind == 2u) { r5_unreported[e] = 0; if (j >= 0 && j < JOB_MAX) r5_last_error[j] = e; }
 }
 if (r5_cache_trace_frozen) return;
 p = r5_cache_trace[r5_cache_trace_count % R5_TRACE_ROWS];
 p[0] = ++r5_cache_trace_count; p[1] = kind;
 p[2] = (unsigned int) j; p[3] = job_current_id();
 p[4] = j >= 0 && j < JOB_MAX ? r5_read_seq[j] : 0;
 p[5] = e; p[6] = e < nent ? ce[e].gen : 0;
 p[7] = e < nent ? ce[e].lba : 0; p[8] = e < nent ? ce[e].seq : 0;
 p[9] = e < nent ? ce[e].rmask : 0;
#if JOB_MAX > 32
 p[10] = e < nent ? ce[e].rmask_hi : 0;
#else
 p[10] = 0;
#endif
 p[11] = extra;
 if (kind == 3u || (kind == 2u && !extra)) r5_cache_trace_frozen = kind;
}
static void r5_read_begin(int j)
{
 if (j >= 0 && j < JOB_MAX && !r5_read_active[j]) {
  r5_read_seq[j]++; r5_read_active[j] = 1; r5_last_error[j] = nent;
 }
}
static void r5_read_end(int j, int rc)
{
 if (j >= 0 && j < JOB_MAX) {
  if (rc < 0) r5_trace(5u, r5_last_error[j], (unsigned int) rc);
  r5_read_active[j] = 0;
 }
}
#define R5_TRACE(k,e,x) r5_trace(k,e,x)
#define R5_READ_BEGIN(j) r5_read_begin(j)
#define R5_READ_END(j,r) r5_read_end(j,r)
