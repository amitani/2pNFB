#ifndef SHARED_MMAP
#define SHARED_MMAP

#include <stdint.h>
#include <sstream>
#include <vector>

#define NOMINMAX
#include <windows.h>

int opencv_size_of(int opencv_data_type);

struct image_header_t{
	int repetition;       // SI4.loopRepeatsDone
	int frame_tag;        // frameTag / SI4.acqFramesDoneTotal
	char full_file_name[260];        // SI4.loggingFullFileName()
	double scanimage_processed_time; // now
	double fastz;
	unsigned int height;
	unsigned int width;
	unsigned int channel;
	int opencv_data_type; 
};

struct image_t{
	image_header_t header;
	std::vector<char> data;
    char* p_at(int c,int y, int x);
    char* p_at(int c,int x);
    void resize(int c,int y, int x,int opencv_data_type);
};




struct mmap_header_t{
	unsigned int size;
	bool is_read;
};

class shared_mmap_base
{
protected:
	HANDLE h_mapfile;
	HANDLE h_mutex;
	
	void* p_buf; // pointer to the beginning of memory map
	mmap_header_t* p_mmap_header;
    char* p_mmap_data;
    
	int mmap_size;

	std::ostringstream error;
	bool not_read;

	bool wait_for_mutex(void);
	bool release_mutex(void);

	void clear(void);

	std::string mutex_name;
	std::string mmap_name;

	bool initialize(std::string base_name, unsigned int max_data_size, bool only_if_exist = false);

//     void unsafe_copy_data_to(void* destination);
//     void unsafe_copy_data_to(void* destination,unsigned int length);
    
	static const int mutex_timeout = 50;
	static const int new_image_timeout = 500;
    
    shared_mmap_base();
public:
	shared_mmap_base(std::string base_name, unsigned int max_data_size);
	~shared_mmap_base();

	bool is_valid(void);
	const std::string get_error(void);
	const std::string get_error_and_clear(void);
    
    bool copy_data_from(char* source);
    bool copy_data_to(char* destination, bool to_wait_new);
};


class shared_mmap : public shared_mmap_base
{
private:
    image_header_t* p_mmap_image_header;
    char* p_mmap_image_data;
	
	bool initialize(std::string base_name, unsigned int max_image_data_size, bool only_if_exist = false);

    void unsafe_copy_image_data_to(image_t* destination);

public:
    int get_max_data_size(void);
    
	shared_mmap(std::string base_name, unsigned int max_image_data_size);

    bool copy_image_data_from(const image_t& source);
    bool copy_image_data_from(const image_header_t &source, const char* p_data, const int length);
    bool copy_image_data_to(image_t* destination);
    bool copy_image_data_to(image_t* destination, bool to_wait_new);
};
#endif