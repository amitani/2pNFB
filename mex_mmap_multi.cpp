#include "mex.h"
#include "shared_mmap.h"
#include <map>
#include <string>
#include <memory>

static std::map<std::string, std::shared_ptr<shared_mmap>> map_mmap;

int get_opencv_type(mxClassID classID){
	switch(classID){
	case mxINT8_CLASS:
		return 0;
	case mxUINT8_CLASS:
		return 1;
	case mxINT16_CLASS:
		return 2;
	case mxUINT16_CLASS:
		return 3;
	case mxINT32_CLASS:
		return 4;
	case mxSINGLE_CLASS:
		return 5;
	case mxDOUBLE_CLASS:
		return 6;
	default:
		return -1;
	}
}

mxClassID get_matlab_type(int opencv_type){
	switch(opencv_type){
	case 0:
		return mxINT8_CLASS;
	case 1:
		return mxUINT8_CLASS;
	case 2:
		return mxINT16_CLASS;
	case 3:
		return mxUINT16_CLASS;
	case 4:
		return mxINT32_CLASS;
	case 5:
		return mxSINGLE_CLASS;
	case 6:
		return mxDOUBLE_CLASS;
	default:
		return mxUNKNOWN_CLASS;
	}
}

void clear(void){
//     for (std::map<std::string,std::shared_ptr<shared_mmap>>::iterator it=map_mmap.begin(); it!=map_mmap.end(); ++it){
// 		delete it->second;
//         it->second = NULL;
//     }
    map_mmap.clear();
	if(mexIsLocked()) mexUnlock();
};

void print_mmap_error(){
    for (std::map<std::string,std::shared_ptr<shared_mmap>>::iterator it=map_mmap.begin(); it!=map_mmap.end(); ++it){
		if (it->second){
			std::string error = it->second->get_error_and_clear();
			if (!error.empty()) mexWarnMsgTxt(error.c_str());
		}
    }
}

void exit_with_error(std::string error_string){
	print_mmap_error();
	mexErrMsgTxt(error_string.c_str());
}

void mexFunction(int nlhs,mxArray *plhs[],int nrhs, const mxArray *prhs[]){
	
	if(nrhs<1) exit_with_error("Require first argument (command).");
	if(mxGetClassID(prhs[0])!=mxCHAR_CLASS) exit_with_error("First argument should be a string.");
	char * p_cmd = mxArrayToString(prhs[0]);
    std::string cmd(p_cmd);
    mxFree(p_cmd);
    
	if(cmd == "clear"){
		clear();
	}else{
        if(nrhs<2 || mxGetClassID(prhs[1])!=mxCHAR_CLASS) exit_with_error("Require second argument (base name).");
        char * p_basename = mxArrayToString(prhs[1]);
        std::string basename = p_basename;
        mxFree(p_basename);
            
//         if(map_mmap.find(basename) == map_mmap.end())  // no need because shared_ptr() return null
//             map_mmap[basename]=NULL;
        
        if(cmd ==  "init"){
            int max_data_size=0;
            if(nrhs>2){
				if (mxGetClassID(prhs[2]) != mxDOUBLE_CLASS) exit_with_error("Third argument should be a double.");
				void *pr = mxGetData(prhs[2]);
				if (!pr) exit_with_error("No real data in the third argument.");
                max_data_size = *(reinterpret_cast<double*>(pr));
            }
			if (map_mmap.find(basename) != map_mmap.end()) exit_with_error("Already initialized.");

            map_mmap[basename] = std::make_shared<shared_mmap>(basename, max_data_size);
            if(map_mmap[basename]->is_valid()){
                if(!mexIsLocked()) mexLock();
            }else{
                mexPrintf(map_mmap[basename]->get_error_and_clear().c_str());
                map_mmap.erase(basename);
                exit_with_error("Error occured during MMap initialization.");
            }
        }else if(cmd == "is_valid"){
            plhs[0] = mxCreateLogicalScalar(map_mmap.find(basename) != map_mmap.end() && map_mmap[basename]->is_valid());
        }else if(cmd =="set"){
			if (map_mmap.find(basename) == map_mmap.end()) exit_with_error("MMap is not initialized.");

            image_header_t image_header;
            char* p_data;
            int length;

            if(nrhs>2){
                image_header.opencv_data_type = get_opencv_type(mxGetClassID(prhs[2]));
                if(image_header.opencv_data_type<0) exit_with_error("Third argument is not a valid type.");

                mwSize n_dimensions = mxGetNumberOfDimensions(prhs[2]);
                if(n_dimensions>3) exit_with_error("Third argument should be less than 4-D.");
                const mwSize* p_size = mxGetDimensions(prhs[2]);
                if(n_dimensions == 3) image_header.channel = p_size[2];
                else image_header.channel = 1;
                image_header.height = p_size[0];
                image_header.width = p_size[1];

				void *pr = mxGetData(prhs[2]);
				if (!pr) exit_with_error("No real data in the third argument.");
                p_data = reinterpret_cast<char*>(pr);
                length = mxGetNumberOfElements(prhs[2]) * opencv_size_of(image_header.opencv_data_type);
            }else{
                exit_with_error("Image data required.");
            }
            if(nrhs>3){
				if (mxGetClassID(prhs[3]) != mxDOUBLE_CLASS) exit_with_error("Forth argument should be a double.");
				void *pr = mxGetData(prhs[3]);
				if (!pr) exit_with_error("No real data in the forth argument.");
                image_header.repetition = *(reinterpret_cast<double*>(pr));
            }else image_header.repetition  = 0;
            if(nrhs>4){
                if(mxGetClassID(prhs[4])!=mxDOUBLE_CLASS) exit_with_error("Fifth argument should be a double.");
				void *pr = mxGetData(prhs[4]);
				if (!pr) exit_with_error("No real data in the fifth argument.");
                image_header.frame_tag = *(reinterpret_cast<double*>(pr));
            }else image_header.frame_tag = 0; 
            if(nrhs>5){
				if (mxGetClassID(prhs[5]) != mxDOUBLE_CLASS) exit_with_error("Sixth argument should be a double.");
				void *pr = mxGetData(prhs[5]);
				if (!pr) exit_with_error("No real data in the sixth argument.");
                image_header.scanimage_processed_time = *(reinterpret_cast<double*>(pr));
            }else image_header.scanimage_processed_time = 0;
            if(nrhs>6){
                if(mxGetClassID(prhs[6])!=mxCHAR_CLASS) exit_with_error("Seventh argument should be a string.");
                std::string full_file_name = mxArrayToString(prhs[6]);
                image_header.full_file_name[259] = '\0';
                strncpy(image_header.full_file_name, full_file_name.c_str(), 259);
            }else strcpy(image_header.full_file_name, "");
            if(nrhs>7){
				if (mxGetClassID(prhs[7]) != mxDOUBLE_CLASS) exit_with_error("Eighth argument should be a double.");
				void *pr = mxGetData(prhs[7]);
				if (!pr) exit_with_error("No real data in the eighth argument.");
                image_header.fastz = *(reinterpret_cast<double*>(pr));
            }else image_header.fastz = 0;

            if(!map_mmap[basename]->copy_image_data_from(image_header, p_data, length)) exit_with_error("Could not set image.");

        }else if(cmd == "get" || cmd == "getnew" ){
            bool to_wait_new = cmd == "getnew";
			if (map_mmap.find(basename) == map_mmap.end()) exit_with_error("MMap is not initialized.");
            image_t result_image;
            result_image.header.repetition = 0;
            result_image.header.frame_tag = 0;
            result_image.header.full_file_name[0] = 0;
            result_image.header.scanimage_processed_time = 0;
            result_image.header.fastz = 0;
            result_image.header.height = 0;
            result_image.header.width = 0;
            result_image.header.channel = 0;
            result_image.header.opencv_data_type = 0;
            /*if(!map_mmap[basename]->copy_image_data_to(&result_image, to_wait_new)){
//                 exit_with_error("Error/Timeout occured during copying from MMap.");
                plhs[0] = mxCreateNumericMatrix (0,0,mxDOUBLE_CLASS , mxREAL);
            }else*/{
                bool is_new = map_mmap[basename]->copy_image_data_to(&result_image, to_wait_new);
                const char* field_names[7] = {"is_new","repetition", "frame_tag",
                    "full_file_name", "scanimage_processed_time", "fastz","raw_intensity"};
                
                plhs[0]=mxCreateStructMatrix(1, 1, 7, field_names);

                mxSetField(plhs[0], 0, field_names[0], mxCreateDoubleScalar(is_new?1:0));
                mxSetField(plhs[0], 0, field_names[1], mxCreateDoubleScalar(result_image.header.repetition));
                mxSetField(plhs[0], 0, field_names[2], mxCreateDoubleScalar(result_image.header.frame_tag));
                mxSetField(plhs[0], 0, field_names[3], mxCreateString(result_image.header.full_file_name));
                mxSetField(plhs[0], 0, field_names[4], mxCreateDoubleScalar(result_image.header.scanimage_processed_time));
                mxSetField(plhs[0], 0, field_names[5], mxCreateDoubleScalar(result_image.header.fastz));
                
                mxArray* p_matrix;
                mwSize return_size[] = {result_image.header.height,result_image.header.width,result_image.header.channel};
                p_matrix = mxCreateNumericArray (3, return_size, get_matlab_type(result_image.header.opencv_data_type), mxREAL);
                std::copy(result_image.data.begin(),result_image.data.end(),reinterpret_cast<char*>(mxGetData(p_matrix)));
                mxSetField(plhs[0], 0, field_names[6], p_matrix);
            }
        }else if(cmd == "getdata" || cmd == "getdatanew"){
            bool to_wait_new = cmd == "getdatanew";
			if (map_mmap.find(basename) == map_mmap.end()) exit_with_error("MMap is not initialized.");
            image_t result_image;
            if(!map_mmap[basename]->copy_image_data_to(&result_image, to_wait_new)){
//                 exit_with_error("Error/Timeout occured during copying from MMap.");
                plhs[0] = mxCreateNumericMatrix (0,0,mxDOUBLE_CLASS , mxREAL);
            }else{
                mwSize return_size[] = {result_image.header.height,result_image.header.width,result_image.header.channel};
                plhs[0] = mxCreateNumericArray (3, return_size, get_matlab_type(result_image.header.opencv_data_type), mxREAL);
                std::copy(result_image.data.begin(),result_image.data.end(),reinterpret_cast<char*>(mxGetData(plhs[0])));
            }
        }else{
            exit_with_error("Unknown command.");
        }
    }
	print_mmap_error();
}




