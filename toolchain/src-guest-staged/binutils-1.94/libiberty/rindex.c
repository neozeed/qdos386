/* Stub implementation of (obsolete) rindex(). */

char *
rindex (s, c)
  char *s;
  int c;
{
  return strrchr (s, c);
}
