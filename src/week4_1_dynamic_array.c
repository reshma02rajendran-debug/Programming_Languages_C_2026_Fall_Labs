#include <stdio.h>
#include <stdlib.h>
int main(){
    int a;
    int *array = NULL;
    int sum = 0;
    float average = 0.0;
    printf("Enter number of elements: ");
    if (scanf("%d", &a) != 1 || a <= 0) {
        printf("Invalid size.\n");
        return 1; }
    array = (int *)malloc(a * sizeof(int));
    if (array == NULL) {
        printf("Memory allocation failed.\n");
        return 1;
    }
    printf("Enter %d integers: ", a);
    for (int i = 0; i < a; i++) {
        if (scanf("%d", &array[i]) != 1) {
            printf("Invalid input.\n");
            free(array); 
            return 1;
        }
        sum += array[i];
    }
    average = (float)sum / a;
    printf("Sum = %d\n", sum);
    printf("Average = %.2f\n", average);
    free(array);
    return 0;
}