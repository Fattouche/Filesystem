## Filesystem

### Usage

1. execute `make`
2. Try each operation

### Operations

#### stat

Displays the information about the file system.

`./statuvfs --image IMAGES/disk03.img`

-----------------------------------



#### ls

Displays the files that exist within the file system.

`./lsuvfs --image IMAGES/disk03.img`

-----------------------------------



#### cat
Displays the contents of a file that exists within the file system.

 `./catuvfs --image IMAGES/disk03.img --file digits_short.txt`

 -----------------------------------



#### store
Store the contents of a file from the host machine to the file system.

`./storuvfs --image disk03.img --file example.txt --source example.txt` 

-----------------------------------



#### download
Download all the files from a file system into the specified directory.

`./downloaduvfs --image IMAGES/disk03.img --directory disk03_output_directory`


