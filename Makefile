CC = gcc
CFLAGS = -Wall -Wextra -g -O2
LDFLAGS = -lpthread -lm -lcrypto

SRCDIR = src
TESTDIR = tests

OBJ = $(SRCDIR)/wire.o \
      $(SRCDIR)/cache.o \
      $(SRCDIR)/resolver.o \
      $(SRCDIR)/policy.o \
      $(SRCDIR)/active_defense.o \
      $(SRCDIR)/doh.o \
      $(SRCDIR)/dnssec.o \
      $(SRCDIR)/main.o \
      $(SRCDIR)/rate_limit.o \
      $(SRCDIR)/reputation.o \
      $(SRCDIR)/processor.o
TARGET = dns-firewall
TEST_TARGET = test_wire

all: $(TARGET) $(TEST_TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(TEST_TARGET): $(SRCDIR)/wire.o $(TESTDIR)/test_wire.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Generic compilation rule
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Explicit dependencies
$(SRCDIR)/wire.o: $(SRCDIR)/wire.c $(SRCDIR)/wire.h
$(SRCDIR)/cache.o: $(SRCDIR)/cache.c $(SRCDIR)/cache.h
$(SRCDIR)/resolver.o: $(SRCDIR)/resolver.c $(SRCDIR)/resolver.h $(SRCDIR)/wire.h $(SRCDIR)/cache.h
$(SRCDIR)/policy.o: $(SRCDIR)/policy.c $(SRCDIR)/policy.h $(SRCDIR)/wire.h
$(SRCDIR)/active_defense.o: $(SRCDIR)/active_defense.c $(SRCDIR)/active_defense.h
$(SRCDIR)/doh.o: $(SRCDIR)/doh.c $(SRCDIR)/doh.h $(SRCDIR)/wire.h $(SRCDIR)/resolver.h $(SRCDIR)/cache.h
$(SRCDIR)/dnssec.o: $(SRCDIR)/dnssec.c $(SRCDIR)/dnssec.h $(SRCDIR)/wire.h
$(SRCDIR)/main.o: $(SRCDIR)/main.c $(SRCDIR)/wire.h $(SRCDIR)/resolver.h $(SRCDIR)/cache.h $(SRCDIR)/policy.h $(SRCDIR)/active_defense.h $(SRCDIR)/doh.h $(SRCDIR)/dnssec.h $(SRCDIR)/rate_limit.h $(SRCDIR)/reputation.h
$(SRCDIR)/rate_limit.o: $(SRCDIR)/rate_limit.c $(SRCDIR)/rate_limit.h
$(SRCDIR)/reputation.o: $(SRCDIR)/reputation.c $(SRCDIR)/reputation.h
$(SRCDIR)/processor.o: $(SRCDIR)/processor.c $(SRCDIR)/processor.h $(SRCDIR)/wire.h $(SRCDIR)/resolver.h $(SRCDIR)/cache.h $(SRCDIR)/policy.h $(SRCDIR)/active_defense.h $(SRCDIR)/dnssec.h $(SRCDIR)/rate_limit.h $(SRCDIR)/reputation.h

clean:
	rm -f $(SRCDIR)/*.o $(TESTDIR)/*.o $(TARGET) $(TEST_TARGET)
