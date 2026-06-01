# save as setup.sh
#!/bin/bash
LOOP=$(sudo losetup -f --show -P disk.img)
echo "Using $LOOP"
sudo mount ${LOOP}p1 /tmp/prajna
sudo mkdir -p /tmp/prajna/boot/grub
sudo cp iso/boot/kernel.bin /tmp/prajna/boot/
sudo cp boot/grub/grub.cfg /tmp/prajna/boot/grub/grub.cfg
echo "Hello from FAT32" | sudo tee /tmp/prajna/TEST.TXT
sudo umount /tmp/prajna
sudo losetup -d $LOOP