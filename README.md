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

You can get a sense for how the project works by perusing the examples in the
``examples`` directory. Build the examples by running:

```sh
$ make                     # Build all of the examples
$ make bin/cmdline-usage   # Build a single example
```

## Documentation

Documentation can be viewed [online](https://makinori.dev) or generated locally.
To do the latter, first install [Sphinx](https://www.sphinx-doc.org/en/master/).
Then run the following:

```sh
$ make docs
$ docs/_build/server  # Also runs makinori!
```

## In The Wild

The following is a list of projects using **makinori**:

* <https://www.makinori.dev>
* <https://www.jrpotter.com>
