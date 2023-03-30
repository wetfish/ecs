'''
GoPro Clipper
This program will record video clips when a PIR sensor is triggered. The PIR sensor is connected via an Arduino. 
The Arduino is connected to the computer via USB. The Arduino is programmed to send a signal to the computer when the PIR sensor is triggered. 
The computer then records a video clip. The video clips are saved to the output directory.

The arduino should be programmed to send a 1 when the PIR sensor is triggered and a 0 when the PIR sensor is not triggered.
'''

# import the necessary packages
from keyclipwriter import KeyClipWriter
from imutils.video import VideoStream
import argparse
import datetime
import time
import cv2
from timeit import default_timer as timer
from get_ch import Get_Ch # to trigger recording for testing

class Clipper:
    def __init__(self, output_dir="output1", codec="XVID", buffer_size=60, cam_num=0, lead_len=-1):
        self.output_dir = output_dir
        self.codec = codec
        
        #self.default_clip_len = buffer_size * 20
        self.cam_num = cam_num
        self.clip_lead_len = lead_len # if -1, then use buffer_size to determine clip lead and trail length

        # initialize the video stream and allow the camera sensor to warm up
        print("[INFO] warming up camera...")

        # cv2 videocapture object is just to get the FPS of the camera
        self.cap = cv2.VideoCapture(self.cam_num)
        self.framesps = self.cap.get(cv2.CAP_PROP_FPS)
        self.spframe = 1/self.framesps
        print(f"Cam FPS: {self.framesps}")
        self.cap.release()

        # set buffer size to clip lead length if clip length is specified
        if (self.clip_lead_len is not -1):
            self.buffer_size = int(self.clip_lead_len * self.framesps)
        else:
            self.buffer_size = buffer_size

        # we use the videostream object to actually get the frames
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
        # control fps
        if (self.get_frame_time() < self.spframe):
            return True
        
        # grab the current frame, resize it, and initialize a
        # boolean used to indicate if the consecutive frames
        # counter should be updated
        frame = self.vs.read()
        # get current time
        self.frame_time = timer()
        # resize frame
        scale_pcent = 50
        if (scale_pcent is not 100):
            new_w = int(frame.shape[1] * scale_pcent / 100)
            new_l = int(frame.shape[0] * scale_pcent / 100)
            new_shape = (new_w, new_l)
            frame = cv2.resize(frame, new_shape, interpolation=cv2.INTER_AREA)
        else:
            new_shape = (frame.shape[1], frame.shape[0])
        # display the output frame size, but only once
        if (not self.display_res_once):
            print(f"output video size: {new_shape}")
            self.display_res_once = True
       
        # Let buffer fill before doing anything else:
        if (len(self.kcw.frames) < self.kcw.bufSize):
            self.kcw.update(frame)
            return True
        
        # count frames
        self.frame_count += 1

        # check if we should trigger a recording
        [self.recording_trigger, NULL] = self.PIR_sense(frame)

        # check to see if a recording should be started
        if self.recording_trigger:
            # reset the number of consecutive frames without action
            self.consecFrames = 0        
            # if we are not already recording, start recording
            if not self.kcw.recording:
                # uncomment this if using some kind of motion detection as a trigger
                #self.recording_trigger = False # reset trigger
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
        else:
            self.consecFrames += 1

        
        # update the key frame clip buffer
        self.kcw.update(frame)
        # if we are recording and reached a threshold on consecutive
        # number of frames with no action, stop recording the clip
        if self.kcw.recording and self.consecFrames >= self.buffer_size:
            self.kcw.finish()
            print("[INFO] stop recording")

        # get keypress from user
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
        print("Stop video stream")
        self.vs.stop()
        time.sleep(.5) # i think the vs object needs this sleep to finish cleaning up, else it throws

    # returns the time since the last frame was grabbed
    # needed so we don't grab the same frame twice
    def get_frame_time(self):
        return timer() - self.frame_time

    # returns the time since the start of the program
    def get_elapsed_time(self):
        self.now_time = timer()
        return self.now_time - self.start_time
    
    # checks if the PIR sensor is active
    # returns true if active, false if inactive
    # requires a serial connection to an arduino
    def PIR_sense(self, frame):
        data = self.try_read_serial()
        if (type(data) is str):  
            if data == "1":
                self.PIR_active = True
            elif data == "0":
                self.PIR_active = False
        
        # Define text and font
        text = ""
        text_color = (0, 0, 0)
        if self.PIR_active:
            text = "PIR:   ACTIVE"
            text_color = (0, 255, 0)
        else:
            text = "PIR: INACTIVE"
            text_color = (0, 0, 255)
        font = cv2.FONT_HERSHEY_SIMPLEX
        fontScale = .35
        thickness = 1

        # Get text size
        textSize = cv2.getTextSize(text, font, fontScale, thickness)[0]

        # Define text position
        x = frame.shape[1] - textSize[0] - 10
        y = textSize[1] + 10

        # Draw text on image
        cv2.putText(frame, text, (x, y), font, fontScale, text_color, thickness)

        return [self.PIR_active, frame]

if __name__ == "__main__":
    # construct the argument parse and parse the arguments
    ap = argparse.ArgumentParser()
    ap.add_argument("-o", "--output", type=str, default="output1",
        help="path to output directory")
    ap.add_argument("-c", "--codec", type=str, default="XVID",
        help="codec of output video")
    ap.add_argument("-b", "--buffer-size", type=int, default=60,
        help="buffer size of video clip writer")
    ap.add_argument("-s", "--source", type=int, default=42,
        help="source cam for video")
    ap.add_argument("-l", "--length-of-lead-in", type=float, default=-1,
        help="clip's leading and trailing length in seconds") # -1 means use buffer size instead
    args = vars(ap.parse_args())

    clipper = Clipper(args["output"], args["codec"], args["buffer_size"], args["source"], args["length_of_lead_in"])
    while(clipper.clip_frame()):
        pass
    print("exiting gopro clipper")
