# -*-Mode: makefile;-*-

XML_DEFINED := false

ifeq ($(HOST),x86_64-linux)

   ifeq ($(TARGET),x86_64-linux)


        # -------------------
        # Native x86_64 build
        # -------------------
        XML_CFLAGS   := $(shell xml2-config --cflags)
        XML_CXXFLAGS := $(XML_CFLAGS)
        XML_LIBS     := $(shell xml2-config --libs)
        XML_DEFINED  ;= true

   else


        ifeq ($(TARGET),arm_CA9-linux)

            # ------------------------------------------
            # Cross build x86_64-linux -> arm_CA9-linux)
            # ------------------------------------------
            xml_root     := /u1/russell/arm_CA9-linux
            XML_CFLAGS   := -I$(xml_root)/include/libxml2
            XML_CXXFLAGS := $(XML_CFLAGS)
            XML_LIBS     := $(xml_root)/lib/libxml2.so          \
                            $(xml_root)/lib/libz.so             \
                            $(xml_root)/lib/liblzma.so          \
                            -lm -ldl
           XNL_DEFINED  := true

        endif

   endif

 else


     ifeq ($(HOST),arm_CA9-linux)

          ifeq ($(TARGET),arm_CA9-linux)

               # -----------------------------
               # Native build on arm_CA9-linux
               # -----------------------------
              XML_CFLAGS   := $(shell xml2-config --cflags)
              XML_CXXFLAGS := $(XML_CFLAGS)
              XML_LIBS     := $(shell xml2-config --libs)
              XML_DEFINED  := true;

          endif

     endif

endif


ifeq ($(XML_DEFINED),true)
       $(error "Cannot cross build for on an <$(HOST)> for target <$(TARGET)>")
endif


