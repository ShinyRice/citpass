# citpass - "See it pass"

**I've hardly got any code written, I'm not sure how long it will take me to get this to a usable state. Don't depend on this for now.**

This is a simple password manager for Linux, inspired in part by [pass](https://www.passwordstore.org/).

It is similar to pass in the sense that;

- It delegates all encryption to GPG (subject to change)

- Aims to be lean

However, citpass differs from it, by

- Storing all passwords and relevant data associated to each in a single file

- Being written in C99

# What does it do?

For now, I've only thought of making it do five things,

- Creating the file where passwords will be stored, which is hardcoded for now to $HOME/.local/share/citpass/passwords

- Adding passwords to this file. Each password will have metadata associated to it stored in the file too,
with a format taken after KeePass, storing title, username, password, URL and notes about the service

- Listing passwords

- Removing passwords and associated metadata

- Retrieving a password from the file. At first, there will only be the option of printing it to stdout,
but when this program is far along enough, I plan on piping it directly to clipboard, by making use of
xclip in X11 and wl-clipboard in Wayland

# Motivation

I was a tad bothered by a few things about pass, like

- Storing each password in an encrypted file, where the file name is supposed to be the name
of the service it's for. This means that anyone with read access to your filesystem can know at a
glance which services you use. To most other people, it's more of a nitpick rather than relevant metadata
that needs to be hidden, mostly because you can name files whatever you want and then have some sort
of index in physical paper that shows what each file corresponds to. But it seems the developer
somewhat intends the file name to be the service name, so I wanted to try making something with
saner defaults. Saner to me, at least.

- Donenfeld claims pass doesn't have as much bloat as other password managers. While that might be true
(I have no idea, haven't taken a look at its source, but since it doesn't do much, I assume it probably doesn't)
pass needs Bash, and Bash *is* bloat, to me and to other people. As this is written in C, obviously this doesn't
depend on a particular shell.

- I honestly don't think that a password manager needs to manage any sort of synchronization between computers.
Even if said synchronization is mostly delegated to a separate program, as many things in pass are.
Well, folders *do* lend themselves to synchronizing with git, but in citpass' case, having a git
repo for a single file/folder with one file doesn't make a lot of sense.

About that last point, yes, I'm aware there's spm, written in POSIX shell. Its design is, however,
flawed in the same way I described in the first point.

My reasons to start writing my own password manager are feeble, I'll admit that much. I can just work
around what I percieve to be wrong with these. But, I wanted to practice writing a program,
and I didn't want to write something that has been done before to death.

# Building and installation

The only run time dependency there is right now is the C standard library,
currently developing this with glibc in mind. Other C standard libraries
might work, but I haven't tested them out. I'll make sure this isn't specific
to glibc, don't think that'll be hard.

Compile time dependencies are gcc and a C stdlib.

In your shell, change directory to the source folder, and run

```
$ make
```

Then, as root,

```
# make install
```

Optionally, remove the executable file generated,

```
$ make clean
```
