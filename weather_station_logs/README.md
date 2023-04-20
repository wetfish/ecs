# Weather Station Logs

## Format
Each log has a heading that is formatted as follows:

`# Time(s), Sensor Reading 1 (Units), Sensor Reading 2 (Units), ...`

Each line following the heading is a comma separated list of sensor readings which corresponds to the comma separated list of sensor labels in the header. 

Any line starting with the '#' character contains no sensor reading data. Concatenated logs include one such line to indicate locations of concatenation. ex:  
`# ./relative_file_path_to_logs/log_x.txt`
