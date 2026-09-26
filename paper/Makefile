IMAGE := mpark-wg21
BUILD := docker build -q -t $(IMAGE) .
RUN := docker run --rm --user $(shell id -u):$(shell id -g) -v "$(CURDIR):/work" $(IMAGE)

define forward
@$(BUILD) >/dev/null
$(RUN) $@
endef

.DEFAULT_GOAL := all

.PHONY: all html pdf latex clean update
all html pdf latex clean update:
	$(forward)

%.pdf:
	$(forward)

%.html:
	$(forward)

%.latex:
	$(forward)
