# really just some handy scripts...

KEXT=ACPIPoller.kext

.PHONY: all
all:
	xcodebuild $(OPTIONS) -configuration Debug
	xcodebuild $(OPTIONS) -configuration Release

.PHONY: clean
clean:
	xcodebuild -configuration Debug clean
	xcodebuild -configuration Release clean

.PHONY: update_kernelcache
update_kernelcache:
	sudo touch /System/Library/Extensions
	sudo kextcache --system-prelinked-kernel -arch x86_64

.PHONY: install_debug
install_debug:
	sudo cp -R ./Build/Debug/$(KEXT) /System/Library/Extensions
	make update_kernelcache

.PHONY: install
install:
	sudo cp -R ./Build/Release/$(KEXT) /System/Library/Extensions
	make update_kernelcache

.PHONY: distribute
distribute:
	if [ -e ./Distribute ]; then rm -r ./Distribute; fi
	mkdir ./Distribute
	cp -R ./Build/Debug ./Distribute
	cp -R ./Build/Release ./Distribute
	find ./Distribute -path *.DS_Store -delete
	find ./Distribute -path *.dSYM -exec echo rm -r {} \; >/tmp/org.voodoo.rm.dsym.sh
	chmod +x /tmp/org.voodoo.rm.dsym.sh
	/tmp/org.voodoo.rm.dsym.sh
	rm /tmp/org.voodoo.rm.dsym.sh
