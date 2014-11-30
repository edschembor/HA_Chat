/** Doubling array for storing updates **/
typedef struct update_array {
	update array[10];
	int size;
	int element_count;
	int start;
} update_array;

update_array attempt_double(update_array ua)
{
	/** Double when the array is full **/
	if(ua.element_count == ua.size)
	{
		update expanded[2*ua.size];
		ua.size = 2*ua.size;

		for(int i = 0; i < ua.element_count; i++) {
			expanded[i] = ua.array[i];
		}

		ua.array = expanded;
	}

	return ua;
}
