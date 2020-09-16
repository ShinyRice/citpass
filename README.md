# citpass - "See it pass"

**I'm not sure how long it will take me to get this to a usable state. Don't depend on this for now.**

This is a simple password manager for Linux, inspired in part by [pass](https://www.passwordstore.org/).

It is similar to pass in the sense that it aims to be simple, lightweight, as well as having readable code,
allowing for ease of maintenance. However, citpass differs from it, by

- Storing passwords somewhat differently. It does store each password in a separate file, along with
metadata, but filenames are random, and an encrypted index file is used to point the user to the correct
password. This idea has been shamelessly stolen from [pwd.sh](https://github.com/drduh/pwd.sh)

- Being written in C99

- Delegating all encryption to libsodium

# What does it do?

For now, I've only thought of making it do five things,

- Creating the directory where passwords are stored and corresponding index file, which defaults to
$HOME/.local/share/citpass, but the directory path can be set through the environment variable CITPASS_DIR

- Adding passwords to the directory. Each password will have metadata associated to it stored in the file too,
with a format taken after KeePass, storing title, username, password, URL and notes about the service

- Listing all passwords

- Removing passwords and associated metadata

- Retrieving a password. At first, there will only be the option of printing it to stdout,
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

- I honestly don't think that a password manager needs to deal with any sort of synchronization between systems.
Even if said synchronization is mostly delegated to a separate program, as many things in pass are.

About the first point, I know that is covered by [pass-tomb](https://github.com/roddhjav/pass-tomb), but
that is not exactly default behaviour, it requires another separate program (which looks promising actually),
and the cherry on top is that it requires systemd. Since I've already said Bash is bloat, I believe you
can guess correctly what's my opinion on systemd, so let's not get into that.

About the second point, I'm aware there's [tpm](https://github.com/nmeum/tpm/), as well as [spm](https://notabug.org/kl3/spm/),
and a myriad other small/smaller password managers written in POSIX shell. Most of them, however, have
the same "flawed" defaults, in the way I described in the first point. If, ultimately, I give up writing
this utility, I may rewrite it in POSIX shell, making use of programs like grep/ripgrep and gpg.

My reasons to start writing my own password manager are feeble, I'll admit that much. I can just work
around what I percieve to be wrong with these. But, I wanted to learn C through practice, by writing
a useful program (to me), and I didn't want to write something that has been done before to death.

# Building and installation

Run time dependencies are glibc and libsodium. I'll make sure this program doesn't specifically depend
on glibc, don't think that'll be hard.

Compile time dependencies are GCC, Make, glibc, and libsodium.

In your shell, change directory to the source directory, and run

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

# License

All of the code I've written myself is licensed under GPL v3 as shown above. This program uses libsodium,
which is licensed under the [ISC license](https://en.wikipedia.org/wiki/ISC_license).
