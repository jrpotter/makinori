# makinori

> [!WARNING]
> This is very much a work in progress. Versioning will reflect once this is
> production ready.

Welcome to the **makinori** web framework. It is a small but fully-featured
framework, written in C and Lua, in which you can write web applications. It
aspires to be as capable as [Django](https://djangoproject.com) or
[Ruby on Rails](https://rubyonrails.org) but with a significantly smaller
footprint.

## Quickstart

Building **makinori** requires the following:

1. [git](https://git-scm.com/),
1. [make](https://www.gnu.org/software/make/), and
1. [CMake](https://cmake.org/).

Optionally, if interested in building documentation locally, you should install
[Sphinx](https://www.sphinx-doc.org/en/master/). You can now clone the project
and build each of the examples:

```sh
$ git clone --recurse-submodules https://github.com/jrpotter/makinori.git
$ make
$ make docs  # Optional
```

You will find each generated executable under the newly created ``bin``
directory.

## In The Wild

The following is a list of projects using **makinori**:

* <https://www.makinori.dev>
* <https://www.jrpotter.com>
