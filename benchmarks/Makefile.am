benchformats = \
    report.pdf \
    report.ps \
    report.html
benchscripts = run-benchmarks \
				bench_http.sh \
				bench_sms.sh
benchoutputs = \
    $(benchformats)

all:

bench: all $(benchformats) $(SUBDIRS)
report.xml:
	run-benchmarks benchmarks/*.sh

clean: $(SUBDIRS)
	rm -f $(benchoutputs)
