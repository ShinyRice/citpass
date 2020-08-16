# citpass - "See it pass"

# I've hardly got any code written, I'm not sure how long it will take me to get this to a usable state. Don't depend on this for now. 

This is a simple password manager for Linux, inspired in part by pass.

It is similar to pass in the sense that;

- It delegates all encryption to GPG

- Aims to be lean

However, it differs from it in the sense that

- It stores all passwords and relevant data associated to each password in a single file

- Written in C99

# What does it do?

For now, I've only thought of making it do five things

- Creating the file where passwords will be stored, which is hardcoded for now to $HOME/.local/share/citpass/passwords

- Adding passwords to this file. Each password will have metadata associated to it stored in the file as well

- Listing passwords

- Removing passwords and associated metadata

- Retrieving a password from the file. For now, there's only the option of printing it to stdout, but when this program is far along enough, I plan on piping it directly to clipboard

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
(I have no idea, haven't taken a look at its source) pass needs Bash, and Bash *is* bloat, to me and
to other people. As this is written in C, obviously this doesn't need a shell.

About that last point, yes, I'm aware there's spm, written in POSIX shell. Its design is, however,
flawed in the same way I described in the first point.

My reasons to start writing my own password manager are feeble, I'll admit that much. I can just work
around what I percieve to be wrong with these. But, I wanted to practice writing a program,
and I didn't want to write something that has been done before to death.

# Building and installation

In your shell, change directory to the source folder, and run

```
$ make
```

Then, as root, (this hasn't even been done yet)

```
# make install
```

Optionally, remove object files and the executable file generated,

```
$ make clean
```
