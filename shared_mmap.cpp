#include "shared_mmap.h"



char* image_t::p_at(int c,int y, int x){
    if(c<header.channel && x < header.height && y < header.width)
        return (&data[0])+opencv_size_of(header.opencv_data_type)
            *(c*header.height*header.width + x*header.height + y);
    else
        return NULL;
}
char* image_t::p_at(int c,int x){
    if(c<header.channel && x < header.height*header.width)
        return (&data[0])+opencv_size_of(header.opencv_data_type)
            *(c*header.height*header.width + x);
    else
        return NULL;
}
void image_t::resize(int c,int y, int x,int opencv_data_type){
    header.channel = c;
    header.height = y;
    header.width = x;
    header.opencv_data_type = opencv_data_type;
    data.resize(opencv_size_of(opencv_data_type)*c*x*y);
    return;
}


bool shared_mmap_base::is_valid(void){
	return p_buf;
}

const std::string shared_mmap_base::get_error(void){
	return error.str();
}
const std::string shared_mmap_base::get_error_and_clear(void){
	std::string return_string = error.str();
	error.str("");
	error.clear();
	return return_string;
}

bool shared_mmap_base::wait_for_mutex(void){
	bool locked=false;
	switch(WaitForSingleObject(h_mutex,mutex_timeout)){
		case WAIT_ABANDONED:
			error << "Mutex was previously abandoned." << std::endl;
		case WAIT_OBJECT_0:
			locked = true;
			break;
		case WAIT_TIMEOUT:
			error << "Time-out before obtaining a mutex." << std::endl;
		case WAIT_FAILED:
		default:
			error << "Failed to obtain a mutex." << std::endl;
			locked = false;
			break;
	}
	return locked;
}

bool shared_mmap_base::release_mutex(void){
	return ReleaseMutex(h_mutex);
}

void shared_mmap_base::clear(void){
	if(p_buf){
		UnmapViewOfFile(p_buf);
		p_buf=NULL;
	}
	if(h_mapfile){
		CloseHandle(h_mapfile);
		h_mapfile=NULL;
	}
	if(h_mutex){
		ReleaseMutex(h_mutex);
		CloseHandle(h_mutex);
		h_mutex=NULL;
	}
}

shared_mmap_base::~shared_mmap_base(){
	clear();
}

shared_mmap_base::shared_mmap_base(){
}

shared_mmap_base::shared_mmap_base(std::string base_name, unsigned int max_data_size){
	h_mutex = NULL;
	h_mapfile = NULL; 
	p_buf = NULL; 

	initialize( base_name,max_data_size);
}

bool shared_mmap_base::initialize(std::string base_name, unsigned int max_data_size, bool only_if_exist){
	// caused seg fault in some environment. there is a bug in implementation of flexible size
	// VirtualQuery is not guaranteed to return the exact size of the allocated size. 
	// mmap size flexibility should be eliminated, instead with fixed big enough size

	if(is_valid()){
		error << "Already initialized." << std::endl;
		return false;
	}
    if(max_data_size==0) only_if_exist = true;
    
	not_read = false;

	mutex_name = "SHARED_MMAP_" + base_name + "_MUTEX";
	mmap_name = "SHARED_MMAP_" + base_name;
	
	h_mutex = CreateMutex(NULL,false, mutex_name.c_str());
	if(!h_mutex){
		if(GetLastError()==ERROR_ALREADY_EXISTS){
			h_mutex = OpenMutex(SYNCHRONIZE,false, mutex_name.c_str());
			if(!h_mutex){
				error << "Could not open mutex. GLE=" << GetLastError() << std::endl;
				clear();
				return false;
			}
		}else{
			error << "Could not create mutex. GLE=" << GetLastError() << std::endl;
			clear();
			return false;
		}
	}

	int required_mmap_size;
	required_mmap_size = sizeof(mmap_header_t) + max_data_size;
	h_mapfile = OpenFileMapping(FILE_MAP_ALL_ACCESS, false, mmap_name.c_str());
	if (!h_mapfile){
		if (only_if_exist){
			error << "Could not open MMap. GLE=" << GetLastError() << std::endl;
			clear();
			return false;
		}
		else{
			h_mapfile = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, required_mmap_size, mmap_name.c_str());
			if (!h_mapfile){
				error << "Could not open or create MMap. GLE=" << GetLastError() << std::endl;
				clear();
				return false;
			}
		}
	}

	p_buf = MapViewOfFile(h_mapfile,FILE_MAP_WRITE, 0,0,0);
	if(!p_buf){
		error << "Could not obtain MapView. GLE=" << GetLastError() << std::endl;
		clear();
		return false;
	}
	MEMORY_BASIC_INFORMATION  MemInfo; 
	VirtualQuery(p_buf, &MemInfo, sizeof(MemInfo)); 
	mmap_size = MemInfo.RegionSize;

	if (mmap_size < required_mmap_size){
		error << "Size mismatch: opened map size: " << mmap_size << "bytes is smaller than required: " << required_mmap_size << " bytes.\n";
		clear();
		return false;
	}
	
	p_mmap_header=(mmap_header_t*)p_buf;
	p_mmap_header->size = mmap_size;
	p_mmap_header->is_read = true;
	p_mmap_data = (char*)(p_mmap_header + 1);

	return true;
}

bool shared_mmap_base::copy_data_from(char* source){
	if(!is_valid()) return false;

	bool res=false;
	if(wait_for_mutex()){
		if(!p_mmap_header->is_read&&!not_read){
			error << "Previous data is still in the buffer." << std::endl;
			not_read = true;
		}
		not_read = !p_mmap_header->is_read;
		
		std::copy(source,source+p_mmap_header->size,p_mmap_data);
        
		p_mmap_header->is_read = false;
		release_mutex();
		res = true;
	}
	return res;
}



bool shared_mmap_base::copy_data_to(char* destination,bool to_wait_new){
	if(!is_valid()) return false;
	if(to_wait_new){
		ULONGLONG start_time = GetTickCount64();
		while(GetTickCount64() < start_time + new_image_timeout){
			Sleep(1);
			if(wait_for_mutex()){
				if(!p_mmap_header->is_read){
                    std::copy(p_mmap_data,p_mmap_data+p_mmap_header->size,destination);
					release_mutex();
					return true;
				}else{
					release_mutex();
				}
			}
		}
	}else{
		if(wait_for_mutex()){
            std::copy(p_mmap_data,p_mmap_data+p_mmap_header->size,destination);
			release_mutex();
			return true;
		}
	}
	return false;
}









int opencv_size_of(int opencv_data_type){
	switch(opencv_data_type){
	case 0:
		return sizeof(uint8_t);
	case 1:
		return sizeof(int8_t);
	case 2:
		return sizeof(uint16_t);
	case 3:
		return sizeof(int16_t);
	case 4:
		return sizeof(int32_t);
	case 5:
		return sizeof(float);
	case 6:
		return sizeof(double);
	case 7:
		return sizeof(double);
	default:
		return 1;
	}
};


shared_mmap::shared_mmap(std::string base_name, unsigned int max_image_data_size){
	h_mutex = NULL;
	h_mapfile = NULL; 
	p_buf = NULL; 

	initialize( base_name,max_image_data_size);
}

bool shared_mmap::initialize(std::string base_name, unsigned int max_image_data_size, bool only_if_exist){
    if(max_image_data_size==0) only_if_exist = true;
    bool r = shared_mmap_base::initialize(base_name,sizeof(image_header_t) + max_image_data_size, only_if_exist);
	p_mmap_image_header=(image_header_t*)(p_mmap_header+1);
	p_mmap_image_data=(char*)(p_mmap_image_header+1);
    return r;
}

int shared_mmap::get_max_data_size(void){
	return mmap_size - sizeof(mmap_header_t) - sizeof(image_header_t);
}

bool shared_mmap::copy_image_data_from(const image_t& source){
	return copy_image_data_from(source.header, &(source.data[0]), source.data.size());
}

bool shared_mmap::copy_image_data_from(const image_header_t &header, const char* p_data, const int length){
	if(!is_valid()) return false;

	if(sizeof(image_header_t)+sizeof(mmap_header_t)+length > p_mmap_header->size){
		error << "Image is bigger than mmap.";
		return false;
	}

	bool res=false;
	if(wait_for_mutex()){
		if(!p_mmap_header->is_read&&!not_read){
			error << "Previous image is still in the buffer." << std::endl;
			not_read = true;
		}
		not_read = !p_mmap_header->is_read;
		
		*p_mmap_image_header = header;
		std::copy(p_data,p_data+length,p_mmap_image_data);
        
		p_mmap_header->is_read = false;
		release_mutex();
		res = true;
	}else{
    	error << "Could not obtain mutex." << std::endl;
    }
	return res;
}

void shared_mmap::unsafe_copy_image_data_to(image_t* destination){
	int data_size = p_mmap_image_header->height*p_mmap_image_header->width
		*p_mmap_image_header->channel*opencv_size_of(p_mmap_image_header->opencv_data_type);

	destination->header = *p_mmap_image_header;
	destination->data.resize(data_size);
    std::copy(p_mmap_image_data,p_mmap_image_data+data_size,destination->data.begin());
	p_mmap_header->is_read = true;
}

bool shared_mmap::copy_image_data_to(image_t* destination){
	return copy_image_data_to(destination,false);
}

bool shared_mmap::copy_image_data_to(image_t* destination, bool to_wait_new){
	if(!is_valid()){
        destination->header.channel = 0;
        destination->header.height = 0;
        destination->header.width = 0;
        destination->header.opencv_data_type = 0;
        return false;
    }
	if(to_wait_new){
		ULONGLONG start_time = GetTickCount64();
		do{
			if(wait_for_mutex()){
				if(!p_mmap_header->is_read){
					unsafe_copy_image_data_to(destination);
					release_mutex();
					return true;
				}else{
					release_mutex();
				}
			}
			Sleep(1);
		}while(GetTickCount64() < start_time + new_image_timeout);
	}
    if(wait_for_mutex()){
        bool is_read = p_mmap_header->is_read;
        unsafe_copy_image_data_to(destination);
        release_mutex();
        return(!is_read);
    }
    destination->header.channel = 0;
    destination->header.height = 0;
    destination->header.width = 0;
    destination->header.opencv_data_type = 0;
	return false;
}



