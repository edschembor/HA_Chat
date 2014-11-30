/** Doubling array for storing updates **/
typedef struct double_array {
	struct update array[5];
	int size;
	int element_count;
	int start;
} double_array;

double_array attempt_double(double_array da)
{
	/** Double when the array is full **/
	if(element_count == size)
	{
		update[2*da.size] expanded;
		da.size = 2*da.size;

		for(int i = 0; i < element_count; i++) {
			expanded[i] = da.array[i];
		}

		da.array = expanded;
	}

	return da;
}
