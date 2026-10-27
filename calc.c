// Reshma Rajendran;251ADB037.
// https://github.com/reshma02rajendran-debug/Programming_Languages_C_2026_Fall_Labs
// Compile with: gcc -O2 -Wall -Wextra -std=c17 -o calc calc.c -lm
// Resources used: - C Standard Library:
// https://en.cppreference.com/w/c/io/fseek,
// https://en.cppreference.com/w/c/string/byte/strtod
// - Parsing Guide: https://craftinginterpreters.com/parsing-expressions.html
// - Linux man-pages: mkdir(2)
// https://man7.org/linux/man-pages/man2/mkdir.2.html, opendir(3)
// https://man7.org/linux/man-pages/man3/opendir.3.html
// - Algorithms: https://en.wikipedia.org/wiki/Exponentiation_by_squaring
// - AI (Claude), conversation
// link:https://claude.ai/chat/94219592-76e7-47e7-b3b6-bcc2d776ff99
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#define Name "Reshma"
#define Last "Rajendran"
#define ID "251ADB037"
static char* s = NULL;
static long len = 0;
static long p = 0;
static long err = 0;
static char* out = NULL;
static size_t olen = 0;
static size_t ocap = 0;
// AI (Claude): prompt "how does a tokenizer keep track of the position of a
// character in a whole
// file..."https://claude.ai/chat/94219592-76e7-47e7-b3b6-bcc2d776ff99 Source:
// Crafting Interpreters ("Parsing
// Expressions")https://craftinginterpreters.com/parsing-expressions.html I used
// the idea of one index for the whole file so error positions count every
// character.
typedef struct {
  int isf;
  long long i;
  double d;
} Val;
static void fail(long at) {
  if (!err) err = at + 1;
}

static void skip(void) {
  while (s[p] == ' ' || s[p] == '\t') p++;
}
// AI (Claude): prompt "why is doubling size with realloc faster than adding
// fixed amount each
// time?"https://claude.ai/chat/94219592-76e7-47e7-b3b6-bcc2d776ff99 Source:
// cppreference Memory Management,https://en.cppreference.com/w/c/memory/realloc
// I used the doubling idea so adding output stays fast in amortized O(1) time.

static void add(const char* t, size_t n) {
  if (olen + n + 1 > ocap) {
    ocap = (ocap + n + 1) * 2;
    char* tmp = realloc(out, ocap);
    if (!tmp) {
      perror("realloc");
      exit(1);
    }
    out = tmp;
  }
  memcpy(out + olen, t, n);
  olen += n;
}

static double dv(Val v) { return v.isf ? v.d : (double)v.i; }
// AI (Claude): prompt "in c how do i do integer division like 7 / 2 = 3 but
// get 3.5 when one number has a decimal point like 7.0 / 2"
// https://claude.ai/chat/94219592-76e7-47e7-b3b6-bcc2d776ff99 Source: C
// Standard Library / Type Conversions
// (https://en.cppreference.com/w/c/language/conversion) I used the idea of
// staying in integer arithmetic unless one side is a float.
static Val calc(char op, Val a, Val b, long opAt, long rightAt) {
  Val r = {0, {0}};
  if (a.isf || b.isf) {
    r.isf = 1;
    double da = dv(a), db = dv(b);
    if (op == '+')
      r.d = da + db;
    else if (op == '-')
      r.d = da - db;
    else if (op == '*')
      r.d = da * db;
    else if (op == '/') {
      if (db == 0.0)
        fail(rightAt);
      else
        r.d = da / db;
    } else if (op == 'p') {
      r.d = pow(da, db);
      if (isnan(r.d) || isinf(r.d)) fail(opAt);
    }
  } else {
    r.isf = 0;
    if (op == '+')
      r.i = a.i + b.i;
    else if (op == '-')
      r.i = a.i - b.i;
    else if (op == '*')
      r.i = a.i * b.i;
    else if (op == '/') {
      if (b.i == 0)
        fail(rightAt);
      else
        r.i = a.i / b.i;
    } else if (op == 'p') {
      if (b.i < 0)
        fail(rightAt);
      else {
        long long x = a.i, e = b.i;
        r.i = 1;
        while (e) {
          if (e & 1) r.i *= x;
          e >>= 1;
          if (e) x *= x;
        }
      }
    }
  }
  return r;
}

static Val expr(void);

static Val primary(void) {
  skip();
  Val v = {0, {0}};
  if (err) return v;
  if (p >= len || s[p] == '\n') {
    fail(p);
    return v;
  }
  if (s[p] == '(') {
    long opn = p;
    p++;
    v = expr();
    if (err) return v;
    skip();
    if (p < len && s[p] == ')') {
      p++;
    } else {
      fail(p);
    }
    return v;
  }
  if (isdigit((unsigned char)s[p])) {
    long q = p;
    int dot = 0;
    while (p < len && (isdigit((unsigned char)s[p]) || s[p] == '.')) {
      if (s[p] == '.') {
        if (dot || (p + 1 < len && !isdigit((unsigned char)s[p + 1]))) {
          fail(p);
          return v;
        }
        dot = 1;
      }
      p++;
    }
    if (dot) {
      char c = s[p];
      s[p] = 0;
      v.d = strtod(s + q, NULL);
      s[p] = c;
      v.isf = 1;
    } else {
      char c = s[p];
      s[p] = 0;
      v.i = strtoll(s + q, NULL, 10);
      s[p] = c;
      v.isf = 0;
    }
    return v;
  }
  fail(p);
  return v;
}
static Val power(void) {
  Val a = primary();
  if (err) return a;
  skip();
  long at = p;
  if (p + 1 < len && s[p] == '*' && s[p + 1] == '*') {
    p += 2;
    skip();
    long rat = p;
    Val b = power();
    if (err) return a;
    return calc('p', a, b, at, rat);
  }
  return a;
}

static Val term(void) {
  Val a = power();
  if (err) return a;
  while (1) {
    skip();
    if (p >= len || (s[p] != '*' && s[p] != '/')) break;
    char op = s[p];
    long at = p;
    p++;
    skip();
    long rat = p;
    Val b = power();
    if (err) return a;
    a = calc(op, a, b, at, rat);
    if (err) return a;
  }
  return a;
}

static Val expr(void) {
  Val a = term();
  if (err) return a;
  while (1) {
    skip();
    if (p >= len || (s[p] != '+' && s[p] != '-')) break;
    char op = s[p];
    long at = p;
    p++;
    skip();
    long rat = p;
    Val b = term();
    if (err) return a;
    a = calc(op, a, b, at, rat);
    if (err) return a;
  }
  return a;
}

static int run(const char* in, const char* outdir, const char* base) {
  FILE* f = fopen(in, "rb");
  if (!f) {
    fprintf(stderr, "calc: cannot open %s\n", in);
    return 1;
  }
  fseek(f, 0, SEEK_END);
  long len_f = ftell(f);
  rewind(f);
  s = malloc(len_f + 1);
  if (!s) {
    fclose(f);
    return 1;
  }
  long rsz = fread(s, 1, len_f, f);
  s[rsz] = 0;
  fclose(f);
  len = rsz;
  p = 0;
  olen = 0;

  while (p < len) {
    while (p < len && (s[p] == ' ' || s[p] == '\t' || s[p] == '\r')) p++;
    if (p >= len) break;
    if (s[p] == '\n') {
      p++;
      continue;
    }
    if (s[p] == '#') {
      while (p < len && s[p] != '\n') p++;
      if (p < len) p++;
      continue;
    }
    err = 0;
    Val v = expr();
    skip();
    if (!err && p < len && s[p] != '\n' && s[p] != '\r') fail(p);
    if (err) {
      char buf[64];
      int n = snprintf(buf, sizeof buf, "ERROR:%ld\n", err);
      olen = 0;
      add(buf, n);
      break;
    }
    char t[64];
    int n = v.isf ? snprintf(t, sizeof t, "%.15g\n", v.d)
                  : snprintf(t, sizeof t, "%lld\n", v.i);
    add(t, n);
    while (p < len && s[p] != '\n') p++;
    if (p < len && s[p] == '\n') p++;
  }

  int rc = 0;
  char path[4096];
  if (mkdir(outdir, 0777) != 0 && errno != EEXIST) rc = 1;
  snprintf(path, sizeof path, "%s/task1_%s_%s_%s.txt", outdir, base, Last, ID);
  FILE* o = rc ? NULL : fopen(path, "wb");
  if (!o) {
    if (!rc) {
      fprintf(stderr, "calc: cannot open output %s\n", path);
      rc = 1;
    }
  } else {
    fwrite(out, 1, olen, o);
    fclose(o);
  }
  free(s);
  free(out);
  out = NULL;
  ocap = 0;
  return rc;
}
// AI (Claude): prompt "why is ** right associative but + and - are left
// associative and how do i do that in a
// parser?"https://claude.ai/chat/94219592-76e7-47e7-b3b6-bcc2d776ff99
// Reference: https://man7.org/linux/man-pages/man3/opendir.3.html,
// https://man7.org/linux/man-pages/man2/mkdir.2.html I used this to handle
// command-line flags (-d, -o) and iterate through directories.
int main(int argc, char** argv) {
  char *dir = NULL, *outdir = NULL, *in = NULL;
  for (int i = 1; i < argc; i++) {
    if (!strcmp(argv[i], "-d") || !strcmp(argv[i], "--dir")) {
      if (i + 1 < argc) dir = argv[++i];
    } else if (!strcmp(argv[i], "-o") || !strcmp(argv[i], "--output-dir")) {
      if (i + 1 < argc) outdir = argv[++i];
    } else {
      in = argv[i];
    }
  }
  char base[256] = "task1";
  if (in) {
    char* slash = strrchr(in, '/');
    char* name = slash ? slash + 1 : in;
    size_t len_name = strlen(name);
    if (len_name > 4 && !strcmp(name + len_name - 4, ".txt")) {
      strncpy(base, name, len_name - 4);
      base[len_name - 4] = 0;
    } else {
      strncpy(base, name, sizeof base - 1);
    }
  }
  char def[512];
  if (!outdir) {
    if (dir) {
      snprintf(def, sizeof def, "%s_%s_%s", dir, Last, ID);
    } else {
      snprintf(def, sizeof def, "%s_%s_%s", base, Last, ID);
    }
    outdir = def;
  }
  if (in) return run(in, outdir, base);
  DIR* d = opendir(dir);
  if (!d) {
    fprintf(stderr, "calc: cannot open folder %s\n", dir);
    return 1;
  }
  int rc = 0;
  struct dirent* ent;
  while ((ent = readdir(d)) != NULL) {
    if (ent->d_name[0] == '.') continue;
    char sub[4096];
    snprintf(sub, sizeof sub, "%s/%s", dir, ent->d_name);
    struct stat st;
    if (stat(sub, &st) == 0 && S_ISREG(st.st_mode)) {
      size_t n = strlen(ent->d_name);
      if (n > 4 && !strcmp(ent->d_name + n - 4, ".txt")) {
        char b[256];
        strncpy(b, ent->d_name, n - 4);
        b[n - 4] = 0;
        if (run(sub, outdir, b)) rc = 1;
      }
    }
  }
  closedir(d);
  return rc;
}
