## Purpose
provide a solution for querying kernel conntrack record.

## implement
inject_conntrack.ko inject a protocol that contains the conntrack between application layer and transport layer。application program need to decode protoco and them read application data.

## Run

```
# install
make && make install

# uninstall
make install 

```