function copy_frame_to_mmap(src,evnt,varargin)
    persistent fastz;
    persistent fastz_frametag;
    persistent last_frametag;
    persistent frame_count;
    
    hSI = evnt.Source; %Handle to ScanImage
    switch evnt.EventName
        case 'frameAcquired'
            if(~mex_mmap_multi('is_valid','SI4_RAW'))
                mex_mmap_multi('init','SI4_RAW',512*512*2*2);
            end
            image_data = hSI.acqFrameBuffer{1};
            if(size(image_data,3)==1)
                image_data(image_data<80)=mean(image_data(image_data<80)); % to avoid PMT(?) noise.  
                % too slow to process two channels. it has to be incorporated with shared_mmap;
            end
            frame_tag = hSI.acqFramesDoneTotal;
            repetition = hSI.loopRepeatsDone;
            scanimage_processed_time = now();
            if(isempty(fastz) || isempty(fastz_frametag) || frame_tag < fastz_frametag || frame_tag > fastz_frametag + 100)
                tmp = hSI.hFastZ.positionAbsolute;
                fastz = tmp(3);
                fastz_frametag = frame_tag;
            end
            if(isempty(last_frametag)||isempty(frame_count))
                last_frametag=frame_tag;
                frame_count=0;
            end
            if(frame_tag<last_frametag||frame_tag>last_frametag+1000)
                fprintf('%d/%d\n',frame_count,frame_tag-1-last_frametag);
                last_frametag=frame_tag;
                frame_count=0;
            else
                frame_count=frame_count+1;
            end
            full_file_name = hSI.loggingFullFileName;
            mex_mmap_multi('set','SI4_RAW',image_data, repetition,frame_tag, scanimage_processed_time,full_file_name,fastz);
        case 'applicationWillClose'
            mex_mmap_multi('clear','SI4_RAW');
    end
