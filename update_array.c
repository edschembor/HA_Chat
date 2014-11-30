#define INITIAL_SIZE 10

/** Doubling array for storing updates **/
typedef struct update_array {
	update *array; //Pointer to array to deal with re-sizing
	int  size;
	int  element_count;
	int  start;
} update_array;

update_array attempt_double(update_array ua)
{
	/** Double if the array is full **/
	if(ua.element_count == ua.size)
	{
		update *expanded;
		ua.size = ua.size*2;
		expanded = malloc(sizeof(update)*ua.size);

		for(int i = 0; i < ua.element_count; i++) {
			expanded[i] = ua.array[i];
		}

		ua.array = expanded;
	}

	return ua;

}
