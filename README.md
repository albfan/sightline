# Sightline

This is an experiment to extract information from autotools builds.
The idea is to eventially get some cross-reference information as well
as "just work" by looking at the log output from GNOME continuous,
jhbuild, or local make output.


```sh
# Build sightline
cd src
make


# build your project somewhere like...
make V=1 -j12 > /tmp/foo.txt 2>&1

# analyze build output flags and use clang to analyze
./sightline /tmp/foo.txt
```
