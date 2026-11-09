# MeteoBench
**Braindead simple meteorological data plotting and analysis program**

<img alt="file12" src="https://github.com/user-attachments/assets/6ac41a81-4983-4402-bef9-20de5874e7ce" />

MeteoBench's only purpose in life is to read, plot, and analyze .wlog files (my own file format) from my weather station.

To use:
1. Download and unzip samplewlog.zip
2. Download the executable for your system
3. Launch the executable
4. Press Ctrl + O or press the "Open" button to select downloaded .wlog files

Features include:
- Reading wlog v1 files
- Rendering wlog v1 files on a table
- Plotting wlog v1 files
- Min/Max by all files
- Average by all files and by individual files
- File labels in the graph

new!!!:
- Reading wlog v2 files
- Rendering wlog v2 on a table
- Plotting wlog v2 files
- Validation of wlog v1 files (parameters NaN and out of range)
- Validation of wlog v2 files (parameters NaN and out of range, CRC, and rudimentary record signatures)

Features not currently included:
- Proper Analysis (residual plots, dew point, averages, plotting derivatives)

Some basic analysis exists within plots (min, max, avg, avg by file)

Built with Qt6 and QCustomPlot!!! (I hate C++'s std library so much im switching to python /j)
wlogv1 and wlogv2 formats were made by me, all parsing, verification, repair is written by me as well without use of external libraries (except MAYBE algorithm, map, and set, but that's just C++)
Uhm, there's not really much to say I'm going to finish this up later.
Oh yea clicking any table row or anywhere on a plot will select that record on all other graphs and tables so you can compare that record to any other record with the normal cursor.
There's technically touch compatibility, the tooltip shows up when you tap anywhere on the plot and I've used it on my tablet and it's fine, but generally not recommended. It's a horrible experience.

