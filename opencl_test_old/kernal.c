__kernel void doublevalue(__global float *x, __global float *y)
{
  const int i = get_global_id(0);
  y[i] = x[i] * 2;
}