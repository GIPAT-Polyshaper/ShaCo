TEMPLATE = subdirs
SUBDIRS = core app test

app.depends = core
test.depends = core
