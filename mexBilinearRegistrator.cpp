#include "mex.h"
#include "matrix.h"
#include<vector>
#include<stdint.h>

#include "BilinearRegistrator.h"

void mexFunction(int nlhs,mxArray *plhs[],int nrhs, const mxArray *prhs[]){
	if(nrhs<2) mexErrMsgTxt("Require two arguments.");
	
	int n_arg = 0;
	if (mxGetClassID(prhs[n_arg]) != mxINT16_CLASS) mexErrMsgTxt("First argument: not int16.");
	mwSize n_dimensions = mxGetNumberOfDimensions(prhs[n_arg]);
	if (n_dimensions > 2) mexErrMsgTxt("First argument: larger than 2d array.");
	const mwSize* p_size = mxGetDimensions(prhs[n_arg]);
	int x = p_size[0]; int y = p_size[1];
	ImagePointer<int16_t> target;
	target.image = reinterpret_cast<int16_t*> (mxGetData(prhs[n_arg]));
	for(int i=0;i<2;i++)target.size[i]=p_size[i];
	
	n_arg = 1;
	if (mxGetClassID(prhs[n_arg]) != mxINT16_CLASS) mexErrMsgTxt("Second argument: not int16.");
	n_dimensions = mxGetNumberOfDimensions(prhs[n_arg]);
	if (n_dimensions > 3) mexErrMsgTxt("Second argument: larger than 3d array.");
	p_size = mxGetDimensions(prhs[n_arg]);
	int z = n_dimensions>2 ? p_size[2] : 1;

	int16_t* source_base_pointer = reinterpret_cast<int16_t*> (mxGetData(prhs[n_arg]));
	ImagePointer<int16_t> source;
	for (int i = 0; i < 2; i++)source.size[i] = p_size[i];

	int margin = x / 8;
	if (nrhs > 2){
		n_arg = 2;
		if (mxGetClassID(prhs[n_arg]) != mxDOUBLE_CLASS) mexErrMsgTxt("Third argument: not double.");
		margin = *(double*)(mxGetData(prhs[n_arg]));
	}

	if (margin == 0){
		if (p_size[0] < x || p_size[1] < y) mexErrMsgTxt("Second argument: incorrect size.");
	}
	else{
		if (p_size[0] != x || p_size[1] != y) mexErrMsgTxt("Second argument: incorrect size.");
	}
	if (2 * margin >= x || 2 * margin >= y) mexErrMsgTxt("Third argument: too large margin");


	int depth = 3;
	if (nrhs > 3){
		n_arg = 3;
		if (mxGetClassID(prhs[n_arg]) != mxDOUBLE_CLASS) mexErrMsgTxt("Forth argument: not double.");
		depth = *(double*)(mxGetData(prhs[n_arg]));
	}
	int factor = 2;
	if (nrhs > 4){
		n_arg = 4;
		if (mxGetClassID(prhs[n_arg]) != mxDOUBLE_CLASS) mexErrMsgTxt("Fifth argument: not double.");
		factor = *(double*)(mxGetData(prhs[n_arg]));
	}
	bool subpixel = true;
	if (nrhs > 5){
		n_arg = 5;
		if (mxGetClassID(prhs[n_arg]) != mxDOUBLE_CLASS) mexErrMsgTxt("Sixth argument: not double.");
		subpixel = *(double*)(mxGetData(prhs[n_arg]));
	}
	bool nongreedy = true;
	if (nrhs > 6){
		n_arg = 6;
		if (mxGetClassID(prhs[n_arg]) != mxDOUBLE_CLASS) mexErrMsgTxt("Seventh argument: not double.");
		nongreedy = *(double*)(mxGetData(prhs[n_arg]));
	}
	int minimum_depth = 1;
	if (nrhs > 7){
		n_arg = 7;
		if (mxGetClassID(prhs[n_arg]) != mxDOUBLE_CLASS) mexErrMsgTxt("Eith argument: not double.");
		minimum_depth = *(double*)(mxGetData(prhs[n_arg]));
	}





	BilinearRegistrator<int16_t, int64_t> br(target.trim(margin), depth, factor, subpixel, nongreedy, minimum_depth);
    
	plhs[0] = mxCreateNumericMatrix(z,3, mxDOUBLE_CLASS, mxREAL);
	double * rv = (double *)(mxGetData(plhs[0]));

	for(int i=0;i<z;i++){
		double result[3];
		source.image = source_base_pointer + i*p_size[0] * p_size[1];
		br.align(source, &result[0]);
		rv[i + 0 * z] = margin - result[0];
		rv[i + 1 * z] = margin - result[1];
		rv[i + 2 * z] = result[2];
	}
	if (nlhs > 1){
		plhs[1] = z == 1 ? mxCreateNumericArray(2, p_size, mxINT16_CLASS, mxREAL)
			: mxCreateNumericArray(3, p_size, mxINT16_CLASS, mxREAL);
		ImagePointer<int16_t> dist;
		int16_t* dist_base_pointer = reinterpret_cast<int16_t*> (mxGetData(plhs[1]));
		dist.size[0] = p_size[0]; dist.size[1] = p_size[1];

		for (int i = 0; i < z; i++){
			source.image = source_base_pointer + i*p_size[0] * p_size[1];
			dist.image = dist_base_pointer + i*p_size[0] * p_size[1];
			source.shift(dist, -rv[i], -rv[i + z]);
		}
	}
}