# https://pyimagesearch.com/2016/02/29/saving-key-event-video-clips-with-opencv/

# import the necessary packages
from keyclipwriter import KeyClipWriter
from imutils.video import VideoStream
from get_ch import Get_Ch
import argparse
import datetime
import time
import cv2
from timeit import default_timer as timer

class Clipper:
    def __init__(self, output_dir="output1", framesps=30, codec="XVID", buffer_size=30, cam_num=0):
        self.output_dir = output_dir
        self.framesps = framesps
        self.spframe = 1/framesps
        self.codec = codec
        self.buffer_size = buffer_size
        self.default_clip_len = buffer_size * 20
        self.cam_num = cam_num

        # initialize the video stream and allow the camera sensor to
        # warmup
        print("[INFO] warming up camera...")
        self.cap = cv2.VideoCapture(self.cam_num)
        self.framesps = self.cap.get(cv2.CAP_PROP_FPS)
        self.spframe = 1/framesps
        print(f"Cam FPS: {self.framesps}")
        self.cap.release()
        self.vs = VideoStream(self.cam_num,usePiCamera=False).start()
        time.sleep(2.0)



        # initialize key clip writer and the consecutive number of
        # frames that have *not* contained any action
        self.kcw = KeyClipWriter(bufSize=self.buffer_size)
        self.consecFrames = 0
        self.frame_count = 0
        self.recording_trigger = False
        self.start_time = timer()
        self.frame_time = self.start_time
        time.sleep(.01)
        self.now_time = timer()

        self.display_res_once = False

        self.get_key = Get_Ch()

    # this examines one frame
    def clip_frame(self):
        if (self.get_frame_time() < self.spframe):
            return True
        # grab the current frame, resize it, and initialize a
        # boolean used to indicate if the consecutive frames
        # counter should be updated
        frame = self.vs.read()
        #ret, frame = self.cap.read()
        scale_pcent = 50
        new_w = int(frame.shape[1] * scale_pcent / 100)
        new_l = int(frame.shape[0] * scale_pcent / 100)
        new_shape = (new_w, new_l)
        if (not self.display_res_once):
            print(f"output video size: {new_shape}")
            self.display_res_once = True
        frame = cv2.resize(frame, new_shape, interpolation=cv2.INTER_AREA)
        self.frame_time = timer()
        updateConsecFrames = True

        # Let buffer fill before doing anything else:
        if (len(self.kcw.frames) < self.kcw.bufSize):
            self.kcw.update(frame)
            return True
        
        # count frames, and only process every nth frame
        self.frame_count += 1

        # comment out motion detection for now
        #if(self.frame_count % 10 == 0):
            # see if we should record bc of moving thing
            #self.recording_trigger = motion.see_motion(frame)[0] # Return value is a tuple: [bool was_ther_motion, image processed_frame]

        updateConsecFrames = not self.recording_trigger
        if self.recording_trigger:
            # reset the number of consecutive frames
            self.consecFrames = 0        
            # if we are not already recording, start recording
            if not self.kcw.recording:
                #self.recording_trigger = False
                print("[INFO] recording...")
                timestamp = datetime.datetime.now()
                p = "{}/{}.avi".format(self.output_dir,
                    timestamp.strftime("%Y%m%d-%H%M%S"))
                print(p)
                self.kcw.start(p, cv2.VideoWriter_fourcc(*self.codec),
                    self.framesps)

        # otherwise, no action has taken place in this frame, so
        # increment the number of consecutive frames that contain
        # no action
        if updateConsecFrames:
            self.consecFrames += 1
        # update the key frame clip buffer
        self.kcw.update(frame)
        # if we are recording and reached a threshold on consecutive
        # number of frames with no action, stop recording the clip
        if self.kcw.recording and self.consecFrames >= self.buffer_size:
            self.kcw.finish()
            print("[INFO] stop recording")

        key = self.get_key.getch()
        # if the `q` or 'esc' key was pressed, break from the loop
        if key == "q" or key == '\x1b':
            self.end_rec()
            return False
        elif key == "r": # starts a recording
            self.recording_trigger = True
        elif key == "s": # stops a recording
            self.recording_trigger = False
        return True

    def end_rec(self):
        # if we are in the middle of recording a clip, wrap it up
        print("finish key clip writer")
        if self.kcw.recording:
            self.kcw.finish()
            time.sleep(.5)
        print("Stop video stream")
        self.vs.stop()
        time.sleep(.5) # i think the vs object needs this sleep to finish cleaning up, else it throws

    def get_frame_time(self):
        return timer() - self.frame_time

    def get_elapsed_time(self):
        self.now_time = timer()
        return self.now_time - self.start_time

if __name__ == "__main__":
    # construct the argument parse and parse the arguments
    ap = argparse.ArgumentParser()
    ap.add_argument("-o", "--output", type=str, default="output1",
        help="path to output directory")
    ap.add_argument("-f", "--fps", type=int, default=30,
        help="FPS of output video")
    ap.add_argument("-c", "--codec", type=str, default="XVID",
        help="codec of output video")
    ap.add_argument("-b", "--buffer-size", type=int, default=60,
        help="buffer size of video clip writer")
    ap.add_argument("-s", "--source", type=int, default=42,
        help="source cam for video")
    args = vars(ap.parse_args())

    clipper = Clipper(args["output"], args["fps"], args["codec"], args["buffer_size"], args["source"])
    while(clipper.clip_frame()):
        pass
    print("exiting gopro clipper")