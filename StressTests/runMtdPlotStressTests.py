#!/usr/bin/env python

# Define some necessary paths
stressmodule_dir = '../Code/StressTestFramework'
mtdpy_header_dir = '../Code/Mantid/Bin/Shared'
tests_dir = 'MantidScript'

# Import the stress manager definition
import sys
sys.path.append(stressmodule_dir)
import stresstesting

# By default the tests are executed in the  command line environment
# with the output printed to the console but we'll be more verbose here as a demo
mtdplot_runner = stresstesting.MantidPlotTestRunner(mtdpy_header_dir)
console_reporter = stresstesting.TextResultReporter()
mgr = stresstesting.TestManager(tests_dir, mtdpy_header_dir, runner = mtdplot_runner,
                                reporter = console_reporter)
mgr.executeAllTests()
