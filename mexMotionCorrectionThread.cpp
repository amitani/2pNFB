#include "mex.h"

#include <stdlib.h>
#include <string>
#include <process.h> //used for thread
#include <windows.h>
#include <math.h>
#include <stdint.h> 
#include <vector>
#include <deque>
#include <algorithm>
#include <numeric>
#include <fstream>
#include <ctime>
using namespace std;

#include "motionCorrectionThread.h"



shared_ptr<thread_params_t::params> parse_params(int n_arg, int nrhs, const mxArray*prhs[]){
	std::shared_ptr<thread_params_t::params> p_params(new thread_params_t::params);
	
	ImagePointer<int16_t> target_full;
	double margin;
	double depth;
	double factor;
	double subpixel;
	double nongreedy;

	if(nrhs <= n_arg) return NULL;
	{
		if( mxGetClassID(prhs[n_arg]) != mxINT16_CLASS ) {
			return NULL;
		}

		const int n_dim = mxGetNumberOfDimensions(prhs[n_arg]);
		if (n_dim > 2) return NULL;
		const mwSize* m_size = mxGetDimensions(prhs[n_arg]);
		if (!mxGetData(prhs[n_arg])) return NULL;
		target_full.image = (int16_t*)mxGetData(prhs[n_arg]);
		for (int i = 0; i < 2; i++)target_full.size[i] = m_size[i];
	}

	n_arg++;
	if(nrhs <= n_arg) return NULL;
	if( mxGetClassID(prhs[n_arg]) != mxDOUBLE_CLASS ) {
		return NULL;
	}
	if(1!= mxGetNumberOfElements(prhs[n_arg])){
		return NULL;
	}
	if (!mxGetData(prhs[n_arg])) return NULL;
	margin = ((double *)mxGetData(prhs[n_arg]))[0];

	n_arg++;
	if (nrhs <= n_arg) return NULL;
	if (mxGetClassID(prhs[n_arg]) != mxDOUBLE_CLASS) {
		return NULL;
	}
	if (1 != mxGetNumberOfElements(prhs[n_arg])){
		return NULL;
	}
	if (!mxGetData(prhs[n_arg])) return NULL;
	depth = ((double *)mxGetData(prhs[n_arg]))[0];


	n_arg++;
	if (nrhs <= n_arg) return NULL;
	if (mxGetClassID(prhs[n_arg]) != mxDOUBLE_CLASS) {
		return NULL;
	}
	if (1 != mxGetNumberOfElements(prhs[n_arg])){
		return NULL;
	}
	if (!mxGetData(prhs[n_arg])) return NULL;
	factor = ((double *)mxGetData(prhs[n_arg]))[0];

	n_arg++;
	if (nrhs <= n_arg) return NULL;
	if (mxGetClassID(prhs[n_arg]) != mxDOUBLE_CLASS) {
		return NULL;
	}
	if (1 != mxGetNumberOfElements(prhs[n_arg])){
		return NULL;
	}
	if (!mxGetData(prhs[n_arg])) return NULL;
	subpixel = ((double *)mxGetData(prhs[n_arg]))[0];

	n_arg++;
	if (nrhs <= n_arg) return NULL;
	if (mxGetClassID(prhs[n_arg]) != mxDOUBLE_CLASS) {
		return NULL;
	}
	if (1 != mxGetNumberOfElements(prhs[n_arg])){
		return NULL;
	}
	if (!mxGetData(prhs[n_arg])) return NULL;
	nongreedy = ((double *)mxGetData(prhs[n_arg]))[0];


	for (int i = 0; i < 2; i++)if(target_full.size[i] <= 2*margin) return NULL;
	p_params->registrator = make_shared<BilinearRegistrator<int16_t, int64_t>>(target_full.trim(margin), depth, factor,subpixel,nongreedy);
	p_params->margin = margin;

	n_arg++;
	if(nrhs <= n_arg) return NULL;
	if( mxGetClassID(prhs[n_arg]) != mxDOUBLE_CLASS ) {
		return NULL;
	}
	if(1!= mxGetNumberOfElements(prhs[n_arg])){
		return NULL;
	}
	if (!mxGetData(prhs[n_arg])) return NULL;
	p_params->align_channel = ((double *)mxGetData(prhs[n_arg]))[0]-1;
	if (p_params->align_channel < 0){
		return NULL;
	}

	n_arg++;
	if (nrhs <= n_arg) return NULL;
	if (mxGetClassID(prhs[n_arg]) != mxDOUBLE_CLASS) {
		return NULL;
	}
	if (1 != mxGetNumberOfElements(prhs[n_arg])){
		return NULL;
	}
	if (!mxGetData(prhs[n_arg])) return NULL;
	p_params->activity.channel = ((double *)mxGetData(prhs[n_arg]))[0] - 1;
	if (p_params->activity.channel < 0){
		return NULL;
	}


	n_arg++;
	if (nrhs <= n_arg) return NULL;
	if (mxGetClassID(prhs[n_arg]) != mxDOUBLE_CLASS) {
		return NULL;
	}
	if (1 != mxGetNumberOfElements(prhs[n_arg])){
		return NULL;
	}
	if (!mxGetData(prhs[n_arg])) return NULL;
	p_params->activity.n_roi = ((double *)mxGetData(prhs[n_arg]))[0];
	if (p_params->activity.n_roi < 0){
		return NULL;
	}
		
	n_arg++;
	if(nrhs <= n_arg) return NULL;
	if( 0 == mxGetNumberOfElements(prhs[n_arg]) ){
		p_params->activity.roiMasks.clear();
	}else{
		if( mxGetClassID(prhs[n_arg]) != mxDOUBLE_CLASS ) {
			return NULL;
		}

		const int* mat_size = mxGetDimensions(prhs[n_arg]);
        const int n_dim = mxGetNumberOfDimensions(prhs[n_arg]);
        
		if (n_dim != 2 || mat_size[1] != 3){ // pixel_index, n_roi, weight
			return NULL;
		}
		
		{
			if (!mxGetData(prhs[n_arg])) return NULL;
            double* base_pointer = (double*)mxGetData(prhs[n_arg]);
            int m = mxGetM(prhs[n_arg]);
            int n = 3;
            
//             p_params->activity.roiMasks.resize(m*n);
            p_params->activity.roiMasks.assign(base_pointer,base_pointer + m*n);
			for (int i = 0; i < m; i++)
				if (p_params->activity.n_roi < p_params->activity.roiMasks[i + m])
					return NULL;
            
		}
	}
	return p_params;

}



 
// static char* readString(const mxArray *string_array_ptr){
// 	char* buf;
// 	mwSize buflen; 
//   
// 	/* Allocate enough memory to hold the converted string. */ 
// 	buflen = mxGetNumberOfElements(string_array_ptr) + 1;
// 	buf = (char *) mxMalloc(buflen*sizeof(char));
//   
// 	/* Copy the string data from string_array_ptr and place it into buf. */ 
// 	if (mxGetString(string_array_ptr, buf, buflen) != 0)
// 	mexErrMsgIdAndTxt( "MATLAB:explore:invalidStringArray",
// 			"Could not convert string data.");
// 
// 	return buf;
// }
void mexFunction(int nlhs,mxArray *plhs[],int nrhs, const mxArray *prhs[])
{
	static const int max_ROI = 500;

	static thread_params_t* thread_params=NULL;
	static vector<HANDLE> hThread;
	
	int n_arg = 0;
	
	if(nrhs <= n_arg) {
        mexErrMsgTxt("Require input arguments");
    }
	if(  mxGetClassID(prhs[n_arg]) != mxCHAR_CLASS ) {
        mexErrMsgTxt("First input argument should be a string");
    }
	char * p_cmd = mxArrayToString(prhs[n_arg]);
    std::string cmd(p_cmd);
    mxFree(p_cmd);
    
	if(cmd=="run"){
		if(!hThread.empty()) mexErrMsgTxt("Already running. Use update.");
		if(!thread_params)
			thread_params = new thread_params_t;
		else
			mexPrintf("Variables are initialized already. There should be something wrong.");


		thread_params->mmap = new shared_mmap("SI4_RAW",0);
		if(!thread_params->mmap->is_valid()){
			mexPrintf(thread_params->mmap->get_error().c_str());
			delete thread_params;
			thread_params = NULL;
			mexErrMsgTxt("Error in initializing a memory mapped object. Make sure ScanImage4 is properly configured and runnign");
		}
		
		thread_params->mmap_shifted = new shared_mmap("SI4_SHIFTED",thread_params->mmap->get_max_data_size());
		if(!thread_params->mmap_shifted->is_valid()){
			mexPrintf(thread_params->mmap_shifted->get_error().c_str());
			delete thread_params;
			thread_params = NULL;
			mexErrMsgTxt("Error in initializing a memory mapped object.");
		}
		
		
		thread_params->mmap_result = new shared_mmap("ROI_INTENSITIES", max_ROI * sizeof(double));
		if(!thread_params->mmap_result->is_valid()){
			mexPrintf(thread_params->mmap_result->get_error().c_str());
			delete thread_params;
			thread_params = NULL;
			mexErrMsgTxt("Error in initializing a memory mapped object. ");
		}

		thread_params->mmap_average = new shared_mmap("SI4_AVERAGE", thread_params->mmap->get_max_data_size());
		if(!thread_params->mmap_average->is_valid()){
			mexPrintf(thread_params->mmap_average->get_error().c_str());
			delete thread_params;
			thread_params = NULL;
			mexErrMsgTxt("Error in initializing a memory mapped object. ");
		}
        
		thread_params->mmap_dislocation = new shared_mmap("SI4_DISLOCATION", 10 * sizeof(double));
		if(!thread_params->mmap_dislocation->is_valid()){
			mexPrintf(thread_params->mmap_dislocation->get_error().c_str());
			delete thread_params;
			thread_params = NULL;
			mexErrMsgTxt("Error in initializing a memory mapped object. ");
		}

		int nThread = 1;
		if (nrhs>1){
			if (mxGetClassID(prhs[1]) != mxDOUBLE_CLASS){
				mexPrintf("Ignoring the second argument. It should be a double.");
			}
			else{
				nThread = *(reinterpret_cast<double*>(mxGetData(prhs[1])));
			}
		}

		thread_params->p_params =  parse_params(2, nrhs, prhs);
		if(!thread_params->p_params){
			delete thread_params;
			thread_params=NULL;
			mexErrMsgTxt("Invalid arguments.");
		}
		for (int i = 0; i < nThread; i++){
			HANDLE h = (HANDLE)_beginthreadex(NULL, 0, &thread_params_t::SecondThreadFunc, thread_params, 0, NULL);
			if (h){
				hThread.push_back(h);
#ifdef _DEBUG 
				{
					std::stringstream strs;
					strs << "Initiated thread. Thread ID: " << GetThreadId(h) << std::endl;
					mexPrintf(strs.str().c_str());
				}
#endif
			}
			else{
				mexPrintf("failed to start a thread.");
			}
		}
		if (hThread.empty()){
			delete thread_params;
			thread_params = NULL;
			mexErrMsgIdAndTxt("MyToolbox:ImageAnalysis:StartThread", "Error starting a thread.");
		}

		mexLock();
	}else if(cmd=="update"){
		if(hThread.empty()) mexErrMsgTxt("Thread is not running");
		if(!thread_params)
			mexPrintf("Variables are not initialized yet. There should be something wrong.");

		std::shared_ptr<thread_params_t::params> p_params = parse_params(1, nrhs, prhs);
		if(!p_params){
			mexErrMsgTxt("Invalid arguments.");
		}
		thread_params->p_params = p_params;
	}else if(cmd=="get"){
		if(nlhs>2) mexErrMsgTxt("Too many output arguments.");
		
		if(!thread_params) mexErrMsgTxt("No data.");
//		if(hThread.empty()) mexErrMsgTxt("Thread is not running.");
		
		int max_N_frames = thread_params_t::history_length;
		if(nrhs>1){
			if(mxGetClassID(prhs[1])!=mxDOUBLE_CLASS) mexPrintf("Ignoring the second argument. It should be a double.");
			max_N_frames = *(reinterpret_cast<double*>(mxGetData(prhs[1])));
		}

		std::deque<std::shared_ptr<result_t>> list_p_result=thread_params->get_p_result();
		
		std::vector<const char*> field_names;
		field_names.push_back("repetition");
		field_names.push_back("frame_tag");
		field_names.push_back("full_file_name");
		field_names.push_back("scanimage_processed_time");
		field_names.push_back("fastz");
		field_names.push_back("dislocation");
		field_names.push_back("raw_intensity");

		int N_frames = min(max_N_frames, list_p_result.size());
		plhs[0]=mxCreateStructMatrix(N_frames, 1, field_names.size(), &(field_names[0]));

		for(int i=0;i<N_frames;i++){
			mxSetField(plhs[0], i, "repetition", mxCreateDoubleScalar(list_p_result[i]->source_image.header.repetition));
			mxSetField(plhs[0], i, "frame_tag", mxCreateDoubleScalar(list_p_result[i]->source_image.header.frame_tag));
			mxSetField(plhs[0], i, "full_file_name", mxCreateString(list_p_result[i]->source_image.header.full_file_name));
			mxSetField(plhs[0], i, "scanimage_processed_time", mxCreateDoubleScalar(list_p_result[i]->source_image.header.scanimage_processed_time));
			mxSetField(plhs[0], i, "fastz", mxCreateDoubleScalar(list_p_result[i]->source_image.header.fastz));
			
			mxArray* p_matrix;
			p_matrix = mxCreateNumericMatrix(2,1,mxDOUBLE_CLASS,mxREAL);
			std::copy(list_p_result[i]->dislocation,list_p_result[i]->dislocation+2, mxGetPr(p_matrix));
			mxSetField(plhs[0], i, "dislocation", p_matrix);
			
			p_matrix = mxCreateNumericMatrix(list_p_result[i]->raw_intensity.size(),1,mxDOUBLE_CLASS,mxREAL);
			std::copy(list_p_result[i]->raw_intensity.begin(),list_p_result[i]->raw_intensity.end(), mxGetPr(p_matrix));
			mxSetField(plhs[0], i, "raw_intensity", p_matrix);
		}
	}else if(cmd=="get_raw_image"){
		if(nlhs>2) mexErrMsgTxt("Too many output arguments.");
		
		if(!thread_params) mexErrMsgTxt("Not started yet.");
		
		std::deque<std::shared_ptr<result_t>> list_p_result=thread_params->get_p_result();
		
		if(list_p_result.size()==0){
			mexWarnMsgTxt("No data.");
			plhs[0] = mxCreateNumericMatrix (0,0, mxDOUBLE_CLASS, mxREAL);
			return;
		}
		
		mwSize return_size[] = {list_p_result[0]->source_image.header.height,list_p_result[0]->source_image.header.width,
			list_p_result[0]->source_image.header.channel };
		plhs[0] = mxCreateNumericArray (3, return_size, mxINT16_CLASS, mxREAL);
		
		std::copy(list_p_result[0]->source_image.data.begin(), list_p_result[0]->source_image.data.end(), (char*)mxGetData(plhs[0]));
	}else if(cmd=="update_average_image"){
        if(nrhs>1){
			if(mxGetClassID(prhs[1])!=mxDOUBLE_CLASS){
				mexPrintf("Ignoring the second argument. It should be a double.");
			}else{
				thread_params->average_window = *(reinterpret_cast<double*>(mxGetData(prhs[1])));
			}
		}
        HANDLE h = (HANDLE)_beginthreadex(NULL, 0, &updateAverage, thread_params, 0, NULL);
        if (h){
            hThread.push_back(h);
#ifdef _DEBUG 
            {
                std::stringstream strs;
                strs << "Initiated thread. Thread ID: " << GetThreadId(h) << std::endl;
                mexPrintf(strs.str().c_str());
            }
#endif
        }
        else{
            mexWarnMsgTxt("Failed to start a thread.");
        }
    }else if(cmd=="get_average_image"){
		if(nlhs>2) mexErrMsgTxt("Too many output arguments.");
		
		if(!thread_params) mexErrMsgTxt("Not started yet.");
		
		int max_averaging_window = 1;
		if(nrhs>1){
			if(mxGetClassID(prhs[1])!=mxDOUBLE_CLASS){
				mexPrintf("Ignoring the second argument. It should be a double.");
			}else{
				max_averaging_window = *(reinterpret_cast<double*>(mxGetData(prhs[1])));
			}
		}

		std::deque<std::shared_ptr<result_t>> list_p_result=thread_params->get_p_result();
		
		if(list_p_result.size()==0){
			mexWarnMsgTxt("No data.");
			plhs[0] = mxCreateNumericMatrix (0,0, mxDOUBLE_CLASS, mxREAL);
			return;
		}
        
		int averaging_window = max_averaging_window;
        
		if(list_p_result.size() < max_averaging_window){
			std::stringstream ss;
			ss << "Not enough frames in the buffer. Averaging " << list_p_result.size() << " frames." ;
			mexWarnMsgTxt(ss.str().c_str());
			averaging_window = list_p_result.size();
		}

		for(int i=1;i<averaging_window;i++){
			if(list_p_result[0]->shifted_image.header.height != list_p_result[i]->shifted_image.header.height
				||list_p_result[0]->shifted_image.header.width != list_p_result[i]->shifted_image.header.width
				||list_p_result[0]->shifted_image.header.channel != list_p_result[i]->shifted_image.header.channel){
				std::stringstream ss;
				ss << "Could not average all because image sizes do not match. Averaging " << i << " frames." ;
				mexWarnMsgTxt(ss.str().c_str());
				averaging_window = i;
				break;
			}
		}

		mwSize return_size[] = {list_p_result[0]->shifted_image.header.height,list_p_result[0]->shifted_image.header.width,
			list_p_result[0]->shifted_image.header.channel };
		plhs[0] = mxCreateNumericArray (3, return_size, mxDOUBLE_CLASS, mxREAL);
		
		int image_size = list_p_result[0]->source_image.header.height 
			* list_p_result[0]->source_image.header.width;

		for(int i=0;i<averaging_window;i++){
			for(int channel = 0;channel<list_p_result[0]->shifted_image.header.channel;channel++){
				for(int pixel = 0;pixel< image_size;pixel++){
					*(mxGetPr(plhs[0])+channel*image_size+pixel)
						+= double(*((int16_t*)list_p_result[i]->shifted_image.p_at(channel,pixel)))/double(averaging_window);
				}
			}
		}
        /*std::vector<int16_t> tmp;
        for(int channel = 0;channel<list_p_result[0]->shifted_image.header.channel;channel++){
            for(int pixel = 0;pixel< image_size;pixel++){
                tmp.clear();
                tmp.reserve(averaging_window);
                for(int i=0;i<averaging_window;i++){
                    tmp.push_back(*((int16_t*)list_p_result[i]->shifted_image.p_at(channel,pixel)));
				}
                std::nth_element(tmp.begin(), tmp.begin() + tmp.size()/4, tmp.end());
                *(mxGetPr(plhs[0])+channel*image_size+pixel)
						= double(tmp[tmp.size()/4]);
			}
		}*/
        
	}else if (cmd=="isrunning"){
		plhs[0] = mxCreateNumericMatrix(1, 1, mxDOUBLE_CLASS, mxREAL);
		*(reinterpret_cast<double*>(mxGetData(plhs[0]))) = hThread.size();
	}else if(cmd=="stop"){
		if (hThread.empty()){
			mexWarnMsgTxt("Thread is not running.");
		}else{
			thread_params->increase_to_stop(hThread.size());

			DWORD hr = WaitForMultipleObjects(hThread.size(), &hThread[0],true,2000);
			if (hr == WAIT_OBJECT_0){
				for (int i = 0; i < hThread.size();i++) CloseHandle(hThread[i]);
			}else{
				mexPrintf("Thread did not finish properly in 2 sec.");
				for (int i = 0; i < hThread.size(); i++){
					TerminateThread(hThread[i], 1);
					CloseHandle(hThread[i]);
				}
			}
			hThread.clear();
		}

		delete thread_params;
		thread_params=NULL;
				
		mexUnlock();
	}else
		mexErrMsgTxt("First argument is not valid.");
}
