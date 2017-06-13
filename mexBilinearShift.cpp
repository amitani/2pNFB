#include "mex.h"
#include "matrix.h"
#include<vector>
#include<stdint.h>

#include "BilinearRegistrator.h"

void mexFunction(int nlhs,mxArray *plhs[],int nrhs, const mxArray *prhs[]){
	if (nrhs < 2) mexErrMsgTxt("Require two arguments.");
	
	int n_arg = 0;
	mwSize n_dimensions = mxGetNumberOfDimensions(prhs[n_arg]);
	if (n_dimensions > 3) mexErrMsgTxt("First argument: larger than 3d array.");
	if (mxGetClassID(prhs[n_arg]) != mxINT16_CLASS) mexErrMsgTxt("First argument: not int16.");

	const mwSize* p_size = mxGetDimensions(prhs[n_arg]);
	int16_t* source_base_pointer = reinterpret_cast<int16_t*> (mxGetData(prhs[n_arg]));
	int x = p_size[0];
	int y = p_size[1];

	int z;
	if (n_dimensions > 2){
		z = p_size[2];
		plhs[0] = mxCreateNumericArray(3, p_size, mxINT16_CLASS, mxREAL);
	}
	else{
		z = 1;
		plhs[0] = mxCreateNumericArray(2, p_size, mxINT16_CLASS, mxREAL);
	}
	int16_t* dist_base_pointer = reinterpret_cast<int16_t*> (mxGetData(plhs[0]));

	n_arg = 1;
	n_dimensions = mxGetNumberOfDimensions(prhs[n_arg]);
	if (n_dimensions > 2) mexErrMsgTxt("Second argument: larger than 2d array.");
	p_size = mxGetDimensions(prhs[n_arg]);
	if (p_size[0] != z || p_size[1] <2 || p_size[1] > 3) mexErrMsgTxt("Second argument: incorrect size.");
	if (mxGetClassID(prhs[n_arg]) != mxDOUBLE_CLASS) mexErrMsgTxt("Second argument: not double.");
	double *t = reinterpret_cast<double*> (mxGetData(prhs[n_arg]));
    
	ImagePointer<int16_t> dist;
	dist.size[0] = x; dist.size[1] = y;
	ImagePointer<int16_t> source;
	source.size[0] = x; source.size[1] = y;
	
	for (int i = 0; i<z; i++){
		source.image = source_base_pointer + i*x*y;
		dist.image = dist_base_pointer + i*x*y;
		source.shift(dist, -t[i], -t[i+z]);
	}
	return;
}