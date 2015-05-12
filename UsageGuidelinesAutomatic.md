Follow these simple steps to integrate micro-profiler to VisualStudio:
  1. Extract the contents of the deployment package to an empty directory on the target machine;
  1. Make sure no VisualStudio instance is running;
  1. Perform the registration: `regsvr32 micro-profiler.dll`;
  1. Run VisualStudio and go to Tools|Addins... There you may load micro-profiler add-in explicitly and/or specify whether to load it on IDE startup;
  1. Start using micro-profiler!

To add profiling to your application right-click on the target application project in the solution tree and check 'Enable Profiling' checkmark in the context menu. Only 'exe' project types are supported currently.

In order to remove any profiler support trace from your project settings, use 'Remove Profiling Support' context menu.

The profiler' window will be opened automatically at the application startup when it is built with 'Enable Profiling' checkmark. The window is comprised from three lists: the middle list displays all functions (with stats) that are being called, the top list displays functions that call the currently selected function (callers), and the bottom list displays functions that are being called from the currently selected function (callees). You may drill-down or drill-up by double-clicking the callers/callees lists.

Below is a typical layout of the profiler's window with some description of its lists:

![http://micro-profiler.googlecode.com/svn/wiki/img/overview.png](http://micro-profiler.googlecode.com/svn/wiki/img/overview.png)

MicroProfiler UI elements on the picture above:
  1. **Callers statistics** - presents names of the callers that called the last selected function in the base statistics list and the times it was called from the respective caller. Activating (double-click or pressing 'Enter') the function in this list drills statistics down to that function (thus, that function will be selected in the base list);
  1. **Base statistics** - presents statistics of all functions from the profiled scope that were called since the application was started or 'Clear Statistics' button was clicked. Note, that for a function to hit the profile scope, it must reside in a source file compiled with '/GH /Gh' compiler options ('Enable Profiling' context menu turns this options on/off for the project). In order to bookmark necessary functions you can select several functions here via 'ctrl' + 'mouse click'. The callers/callees stats will be displayed for the last selected function only;
  1. **Callees statistics** - presents statistics of all profiled functions in the context of a function currently selected in the base list. Activating (double-click or pressing 'Enter') the function in this list drills statistics down to that function (thus, that function will be selected in the base list);
  1. **Control panel** - with these buttons you can clear the stats collected to moment as well as copy the base statistics to the clipboard. You can later paste this clipboard to a spreadsheet software for further analysis.
