TEMPLATE = subdirs

SUBDIRS += tool \
    parameterizer

SUBDIRS += voronoi
SUBDIRS += voronoi-tester

SUBDIRS += triangle
SUBDIRS += triangle-tester

voronoi-test.depends = voronoi
triangle-tester.depends = triangle
tool.depends = voronoi triangle
