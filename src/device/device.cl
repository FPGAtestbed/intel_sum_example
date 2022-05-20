__kernel
void sum_kernel(__global double * const restrict input, __global double * restrict result, double add_val, int num_its) {
  for(unsigned int i=0;i<num_its;i++) {
    result[i]=input[i]+add_val;
  }
}

