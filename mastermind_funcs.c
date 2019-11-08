#include <stdio.h>
#include <string.h>

#define MMIND_DIGITS 4
#define MMIND_BUFFER_LEN 16

int write_mmind_number(char *, char *, char*, int);

int main(void)
{
  char mmind_number[MMIND_DIGITS+1] = "4283";
  char number[MMIND_DIGITS+1] = { '\0' };
  int num_guess = 0;

  char buffer[16];

  printf("Enter a number: ");
  scanf("%4s", number);

  write_mmind_number(buffer, mmind_number, number, num_guess);

  printf("Buffer: %s\n", buffer);
  printf("\nNum Guess: %d\n", num_guess);
  
  return 0;
}

int write_mmind_number(char *buffer, char *mmind_number, char *number, int num_guess)
{
  int i;
  int m = 0;
  int n = 0;

  printf("MMind number: %s\n", mmind_number);
  
  for (i = 0; i < MMIND_DIGITS; i++) {
    if (mmind_number[i] == number[i]) {
      m++;
    } else {
      if (strchr(mmind_number, number[i])) {
	n++;
      }
    }
  }
  printf("m:%d n:%d\n",m,n);
  snprintf(buffer, MMIND_BUFFER_LEN, "%s %d+ %d- %04d\n ",
	   number, m, n, num_guess);
  num_guess++;

  return num_guess;
}

