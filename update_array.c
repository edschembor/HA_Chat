/** Doubling array for storing updates **/
typedef struct double_array {
	struct update array[];
	int size;
	int element_count;
} double_array;

double_array attempt_double(double_array da)
{
	/** Double when the array is full **/
	if(element_count == size)
	{
		double_array new_da = malloc(sizeof(double_array));
		new_da.size = 
	}
}



/** Pretty sure this file is wrong and we cannot use a struct for this **/
