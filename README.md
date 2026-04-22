# Mini-UnionFS

A lightweight Union Filesystem in C++ using FUSE. Merges a read-only **lower** layer and a writable **upper** layer into a single unified mount point — the same idea behind Docker layers and Linux OverlayFS.

---

## Features

| Feature | Description |
|---|---|
| **Layer Merging** | Files from both layers are visible at the mount point. Upper takes precedence on name conflicts. |
| **Copy-on-Write** | Writing to a lower-layer file silently copies it to the upper layer first; the lower file is never modified. |
| **Whiteouts** | Deleting a lower-layer file creates a `.wh.<name>` marker in the upper layer so the file appears gone in the merged view. |

---

## Build

**Prerequisites:** `g++`, `libfuse-dev`, `pkg-config`

```bash
sudo apt install libfuse-dev pkg-config   # Debian/Ubuntu
make
```

---

## Run

```bash
./bin/unionfs <lowerdir> <upperdir> <mountpoint>
```

**Example:**

```bash
mkdir lower upper mnt
echo "hello" > lower/hello.txt

./bin/unionfs lower/ upper/ mnt/

cat mnt/hello.txt          # reads from lower
echo "world" >> mnt/hello.txt   # CoW: upper/hello.txt created
rm mnt/hello.txt           # whiteout: upper/.wh.hello.txt created

fusermount -u mnt/
```

---

## Test

```bash
bash test_unionfs.sh
```

Runs three automated tests:
1. **Layer Visibility** — lower-layer files visible through the mount
2. **Copy-on-Write** — writes go to upper, lower is untouched
3. **Whiteout** — deletes hide lower files without modifying them

