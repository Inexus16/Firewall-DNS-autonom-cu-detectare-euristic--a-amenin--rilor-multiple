#include "stdafx.h"
#include <Windows.h>
char *tlds[] = {"ru", "info", "biz", "click", "su",
"work", "pl", "org", "pw", "xyz"};
void LockyDGA(char *domain, int pos, int seed,
SYSTEMTIME systemTime)
{
 int v1;
 int v2;
 int v3;
 int v4;
 int v8;
 int v9;
 int v10;
 int v11;
 int v12;
 int v13;
 int v14;
 int v15;
 int v17;
 int v18;
 int v19;
 int v20;
 char *v21;
 int v7;
 unsigned int v5;
 int v6;
 int var18;
 int var14;
 int var10;
 v1 = pos;
 v2 = seed;
 v3 = 0;
 v5 = systemTime.wDay >> 1;
 v4 = systemTime.wYear;
 v1 = _rotl(v1, 0x15);
 v6 = _rotl(v2, 0x11);
 var18 = v6 + v1;
 var14 = v5;
 var10 = 7;
 while (var10 > 0)
  {
 v7 = _rotr(0xB11924E1 * (v4 + v3 + 0x1BF5), 7);
 v8 = (v7 + 0x27100001) ^ v3;
 v9 = _rotr(0xB11924E1 * (v8 + v2), 7);
 v10 = (v9 + 0x27100001) ^ v8;
 v11 = _rotr(0xB11924E1 * (v5 + v10), 7);
 v12 = 0xD8EFFFFF - v11 + v10;
 v13 = _rotr(0xB11924E1 * (systemTime.wMonth + v12
- 0x65CAD), 7); v14 = v12 + v13 + 0x27100001;
 v15 = _rotr(0xB11924E1 * (v14 + var18), 7);
 v3 = (v15 + 0x27100001) ^ v14;
 ++v4;
 var10 = var10 - 1;
 v5 = var14;
 }
 var18 = v3 % 0xBu + 7;
 var10 = 0;
 if (var18 != 0)
 {
 do
 {
 v17 = _rotl(v3, var10);
 v18 = _rotr(0xB11924E1 * v17, 7);
 v3 = v18 + 0x27100001;
 domain[var10++] = v3 % 0x19u + 'a';
 } while (var10 < var18);
 }
 domain[var10++] = '.';
 v19 = _rotr(0xB11924E1 * v3, 7);
 v20 = 0;
 v21 = tlds[(v19 + 0x27100001) % (sizeof(tlds) /
 sizeof(tlds[0]))];
 do
 {
 if (!v21[v20])
 {
 break;
 }
 domain[var10++] = v21[v20++];
 } while (v20 < 5);
}
void showHelpInfo(char *s)
{
 printf("Usage : %s [-option] [argument]\n", s);
 printf("option: -h Show help information\n");
 printf(" -s Seed from Locky Config\n");
 printf(" -d Date with format [yyyy-mm-dd]\n");
 printf(" -n Max count of Domain generated\n");
 printf("Default: -d {current date} -n {7}");
}
int main(int argc, char* argv[])
{
 char domain[40];
 int pos = 0;
 SYSTEMTIME systemTime; int max = 7;
 int seed = 0;
 GetSystemTime(&systemTime);
 if (argc > 1)
 {
 for (int i = 1; i < argc; i++)
 {
 if (i + 1 > argc)
 {
 break;
 }
 if (strcmp(argv[i], "-h") == 0)
 {
 showHelpInfo(argv[0]);
 return 0;
 }
 if (strcmp(argv[i], "-d") == 0)
 {
 char *date = argv[i + 1];
 char buf[5];
 strncpy_s(buf, 5, date, 4);
 if (atoi(buf) != 0) {
 systemTime.wYear = atoi(buf); }
 memset(buf, 0, sizeof(buf));
 strncpy_s(buf, 5, date + 5, 2);
 if (atoi(buf) != 0)
 {
 systemTime.wMonth = atoi(buf); }
 memset(buf, 0, sizeof(buf));
 strncpy_s(buf, 5, date + 8, 2);
 if (atoi(buf) != 0)
 {
 systemTime.wDay = atoi(buf);
 }
 }
 if (strcmp(argv[i], "-n") == 0)
 {
 if (atoi(argv[i + 1]) != 0)
 {
 max = atoi(argv[i + 1]); }
 }
 if (strcmp(argv[i], "-s") == 0)
 {
 if (atoi(argv[i + 1]) != 0)
 {
 seed = atoi(argv[i + 1]);
 }
 }
 }
 }
 do
 {
 memset(domain, 0, sizeof(domain));
 LockyDGA(domain, pos, seed, systemTime);
 printf("DGA %d = %s\n", pos++, domain);
 } while (pos < max);
return 0;
}