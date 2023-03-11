import _thread
import readchar
import termios
import atexit

class Get_Ch:
    def __init__(self, print_stuff = False): # True if you want your key press printed back
        self.user_input = []
        self.print_stuff = print_stuff
        self.old_settings = termios.tcgetattr(0)
        atexit.register(self.restore_terminal_settings)
        _thread.start_new_thread(self.input_thread, (self.user_input,))

    def input_thread(self, kb_in):
        kb_in.append(readchar.readchar())

    def getch(self):
        key = None
        if len(self.user_input) > 0:
            key = self.user_input.pop()
            if(self.print_stuff):
                print(f"{key} pressed.")
            _thread.start_new_thread(self.input_thread, (self.user_input,))
        return key
    
    def restore_terminal_settings(self):
        termios.tcsetattr(0, termios.TCSADRAIN, self.old_settings)

# This just prints back what key you pressed
if __name__ == "__main__":
    getch = Get_Ch(True)
    while 1:
        key = getch.getch()
        if key == 'q' or key == '\x1b':
            break