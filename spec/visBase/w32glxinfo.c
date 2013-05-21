static const char* rcsID = "$Id$";

/*

	wglinfo.c

	Nate Robins, 1997



	Shows a graph of all the visuals that support OpenGL and their

	capabilities.  Just like (well, almost) glxinfo on SGI's.

	Only the -v (verbose) and -h (help) command line arguments are

	implemented.





	A legend for the table that this baby spits out

        -----------------------------------------------



	visual ~= pixel format descriptor

	id      = pixel format number (integer from 1 - max pixel formats)

	dep     = cColorBits      - color depth

	xsp     = no analog       - transparent pixel (currently always ".")

	bfsz    = cColorBits      - framebuffer size (no analog in Win32?)

	lvl     = bReserved       - overlay(>0), underlay(<0), main plane(0).

	rgci    = iPixelType      - rb = rgba mode, ci = color index mode.

	db      = dwFlags & PFD_DOUBLEBUFFER - double buffer flag (y = yes)

	stro    = dwFlags & PFD_STEREO       - stereo flag        (y = yes)

	rsz     = cRedBits        - # bits of red

	gsz     = cGreenBits      - # bits of green

	bsz     = cBlueBits       - # bits of blue

	asz     = cAlphaBits      - # bits of alpha

	axbf    = cAuxBuffers     - # of aux buffers

	dpth    = cDepthBits      - # bits of depth

	stcl    = cStencilBits    - # bits of stencil

	accum r = cAccumRedBits   - # bits of red in accumulation buffer

	accum g = cAccumGreenBits - # bits of green in accumulation buffer

	accum b = cAccumBlueBits  - # bits of blue in accumulation buffer

	accum a = cAccumAlphaBits - # bits of alpha in accumulation buffer

	ms      = no analog  - multisample buffers



 */



#include <windows.h>			/* must include this before GL/gl.h */

#include <GL/gl.h>			/* OpenGL header file */

#include <GL/glu.h>			/* OpenGL utilities header file */

#include <stdio.h>



void

VisualInfo(HDC hDC, int verbose)

{

    int i, maxpf;

    PIXELFORMATDESCRIPTOR pfd;



    /* calling DescribePixelFormat() with NULL args return maximum

       number of pixel formats */

    maxpf = DescribePixelFormat(hDC, 0, 0, NULL);



    if (!verbose) {

 printf("   visual  x  bf lv rg d st  r  g  b a  ax dp st accum buffs  ms \n");

 printf(" id dep cl sp sz l  ci b ro sz sz sz sz bf th cl  r  g  b  a ns b\n");

 printf("-----------------------------------------------------------------\n");



    /* loop through all the pixel formats */

    for(i = 1; i <= maxpf; i++) {



	DescribePixelFormat(hDC, i, sizeof(PIXELFORMATDESCRIPTOR), &pfd);



	/* only describe this format if it supports OpenGL */

	if(!(pfd.dwFlags & PFD_SUPPORT_OPENGL))

	    continue;



	/* other criteria could be tested here for actual pixel format

           choosing in an application:

	   

	   for (...each pixel format...) {



	     if (pfd.dwFlags & PFD_SUPPORT_OPENGL &&

	         pfd.dwFlags & PFD_DOUBLEBUFFER &&

	         pfd.cDepthBits >= 24 &&

	         pfd.cColorBits >= 24)

	         {

	            goto found;

                 }

           }

	   ... not found so exit ...

	    

           found:

	   ... found so use it ...

	*/



	/* print out the information for this pixel format */

	printf("0x%02x ", i);



	printf("%2d ", pfd.cColorBits);

	if(pfd.dwFlags & PFD_DRAW_TO_WINDOW)      printf("wn ");

	else if(pfd.dwFlags & PFD_DRAW_TO_BITMAP) printf("bm ");

	else printf(".  ");



	/* should find transparent pixel from LAYERPLANEDESCRIPTOR */

	printf(" . "); 



	printf("%2d ", pfd.cColorBits);



	/* bReserved field indicates number of over/underlays */

	if(pfd.bReserved) printf(" %d ", pfd.bReserved);

	else printf(" . "); 



	printf(" %c ", pfd.iPixelType == PFD_TYPE_RGBA ? 'r' : 'c');



	printf("%c ", pfd.dwFlags & PFD_DOUBLEBUFFER ? 'y' : '.');



	printf(" %c ", pfd.dwFlags & PFD_STEREO ? 'y' : '.');



	if(pfd.cRedBits && pfd.iPixelType == PFD_TYPE_RGBA) 

	    printf("%2d ", pfd.cRedBits);

	else printf(" . ");



	if(pfd.cGreenBits && pfd.iPixelType == PFD_TYPE_RGBA) 

	    printf("%2d ", pfd.cGreenBits);

	else printf(" . ");



	if(pfd.cBlueBits && pfd.iPixelType == PFD_TYPE_RGBA) 

	    printf("%2d ", pfd.cBlueBits);

	else printf(" . ");



	if(pfd.cAlphaBits && pfd.iPixelType == PFD_TYPE_RGBA) 

	    printf("%2d ", pfd.cAlphaBits);

	else printf(" . ");



	if(pfd.cAuxBuffers)     printf("%2d ", pfd.cAuxBuffers);

	else printf(" . ");



	if(pfd.cDepthBits)      printf("%2d ", pfd.cDepthBits);

	else printf(" . ");



	if(pfd.cStencilBits)    printf("%2d ", pfd.cStencilBits);

	else printf(" . ");



	if(pfd.cAccumRedBits)   printf("%2d ", pfd.cAccumRedBits);

	else printf(" . ");



	if(pfd.cAccumGreenBits) printf("%2d ", pfd.cAccumGreenBits);

	else printf(" . ");



	if(pfd.cAccumBlueBits)  printf("%2d ", pfd.cAccumBlueBits);

	else printf(" . ");



	if(pfd.cAccumAlphaBits) printf("%2d ", pfd.cAccumAlphaBits);

	else printf(" . ");



	/* no multisample in Win32 */

	printf(" . .\n");

    }



    /* print table footer */

 printf("-----------------------------------------------------------------\n");

 printf("   visual  x  bf lv rg d st  r  g  b a  ax dp st accum buffs  ms \n");

 printf(" id dep cl sp sz l  ci b ro sz sz sz sz bf th cl  r  g  b  a ns b\n");

 printf("-----------------------------------------------------------------\n");



    } else {				/* verbose output. */

	/* loop through all the pixel formats */

	for(i = 1; i <= maxpf; i++) {

	    

	    DescribePixelFormat(hDC, i, sizeof(PIXELFORMATDESCRIPTOR), &pfd);

	    

	    /* only describe this format if it supports OpenGL */

	    if(!(pfd.dwFlags & PFD_SUPPORT_OPENGL))

		continue;



	    printf("Visual ID: %2d  depth=%d  class=%s\n", i, pfd.cDepthBits, 

		   pfd.cColorBits <= 8 ? "PseudoColor" : "TrueColor");

	    printf("    bufferSize=%d level=%d renderType=%s doubleBuffer=%d stereo=%d\n", pfd.cColorBits, pfd.bReserved, pfd.iPixelType == PFD_TYPE_RGBA ? "rgba" : "ci", pfd.dwFlags & PFD_DOUBLEBUFFER, pfd.dwFlags & PFD_STEREO);

	    printf("    rgba: redSize=%d greenSize=%d blueSize=%d alphaSize=%d\n", pfd.cRedBits, pfd.cGreenBits, pfd.cBlueBits, pfd.cAlphaBits);

	    printf("    auxBuffers=%d depthSize=%d stencilSize=%d\n", pfd.cAuxBuffers, pfd.cDepthBits, pfd.cStencilBits);

	    printf("    accum: redSize=%d greenSize=%d blueSize=%d alphaSize=%d\n", pfd.cAccumRedBits, pfd.cAccumGreenBits, pfd.cAccumBlueBits, pfd.cAccumAlphaBits);

	    printf("    multiSample=%d multisampleBuffers=%d\n", 0, 0);

	    printf("    Opaque.\n");

	}

    }

}



LONG WINAPI

WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)

{ 

    static PAINTSTRUCT ps;



    switch(uMsg) {

    case WM_PAINT:

	BeginPaint(hWnd, &ps);

	EndPaint(hWnd, &ps);

	return 0;



    case WM_SIZE:

	glViewport(0, 0, LOWORD(lParam), HIWORD(lParam));

	PostMessage(hWnd, WM_PAINT, 0, 0);

	return 0;



    case WM_CHAR:

	switch (wParam) {

	case 27:			/* ESC key */

	    PostQuitMessage(0);

	    break;

	}

	return 0;



    case WM_CLOSE:

	PostQuitMessage(0);

	return 0;

    }



    return (LONG)DefWindowProc(hWnd, uMsg, wParam, lParam); 

} 



HWND

CreateOpenGLWindow(char* title, int x, int y, int width, int height, 

		   BYTE type, DWORD flags)

{

    int         pf;

    HDC         hDC;

    HWND        hWnd;

    WNDCLASS    wc;

    PIXELFORMATDESCRIPTOR pfd;

    static HINSTANCE hInstance = 0;



    /* only register the window class once - use hInstance as a flag. */

    if (!hInstance) {

	hInstance = GetModuleHandle(NULL);

	wc.style         = CS_OWNDC;

	wc.lpfnWndProc   = (WNDPROC)WindowProc;

	wc.cbClsExtra    = 0;

	wc.cbWndExtra    = 0;

	wc.hInstance     = hInstance;

	wc.hIcon         = LoadIcon(NULL, IDI_WINLOGO);

	wc.hCursor       = LoadCursor(NULL, IDC_ARROW);

	wc.hbrBackground = NULL;

	wc.lpszMenuName  = NULL;

	wc.lpszClassName = "OpenGL";



	if (!RegisterClass(&wc)) {

	    MessageBox(NULL, "RegisterClass() failed:  "

		       "Cannot register window class.", "Error", MB_OK);

	    return NULL;

	}

    }



    hWnd = CreateWindow("OpenGL", title, WS_OVERLAPPEDWINDOW |

			WS_CLIPSIBLINGS | WS_CLIPCHILDREN,

			x, y, width, height, NULL, NULL, hInstance, NULL);



    if (hWnd == NULL) {

	MessageBox(NULL, "CreateWindow() failed:  Cannot create a window.",

		   "Error", MB_OK);

	return NULL;

    }



    hDC = GetDC(hWnd);



    /* there is no guarantee that the contents of the stack that become

       the pfd are zeroed, therefore _make sure_ to clear these bits. */

    memset(&pfd, 0, sizeof(pfd));

    pfd.nSize        = sizeof(pfd);

    pfd.nVersion     = 1;

    pfd.dwFlags      = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | flags;

    pfd.iPixelType   = type;

    pfd.cColorBits   = 32;



    pf = ChoosePixelFormat(hDC, &pfd);

    if (pf == 0) {

	MessageBox(NULL, "ChoosePixelFormat() failed:  "

		   "Cannot find a suitable pixel format.", "Error", MB_OK); 

	return 0;

    } 

 

    if (SetPixelFormat(hDC, pf, &pfd) == FALSE) {

	MessageBox(NULL, "SetPixelFormat() failed:  "

		   "Cannot set format specified.", "Error", MB_OK);

	return 0;

    } 



    DescribePixelFormat(hDC, pf, sizeof(PIXELFORMATDESCRIPTOR), &pfd);



    ReleaseDC(hWnd,hDC);



    return hWnd;

}    



int 

main(int argc, char** argv)

{

    HDC hDC;				/* device context */

    HGLRC hRC;				/* opengl context */

    HWND  hWnd;				/* window */

    MSG   msg;				/* message */

    int   i;

    char* s;

    char  t[80];

    char* p;

    int verbose = 0;



    while (--argc) {

	if (strcmp("-h", argv[argc]) == 0) {

	    printf("Usage: wglinfo [-v] [-t] [-m] [-h] [-dispay <dname>] [-nfbc] [-fpcinfo]\n");

	    printf("        -v: Print visuals info in verbose form.\n");

	    printf("        -t: Print verbose table (not implemented on Win32)\n");

	    printf("        -m: Don't print mid table headers (in long tables). (not on Win32)\n");

	    printf("        -display <dname>: Print GLX visuals on specified server. (not on Win32)\n");

	    printf("        -nfbc: Don't use fbconfig extension (not available on Win32)\n");

	    printf("        -fbcinfo: print out additional fbconfig information (not on Win32)\n");

	    printf("        -h: This screen.\n");

	    exit(0);

	} else if (strcmp("-v", argv[argc]) == 0) {

	    verbose = 1;

	}

    }



    hWnd = CreateOpenGLWindow("wglinfo", 0, 0, 100, 100, PFD_TYPE_RGBA, 0);

    if (hWnd == NULL)

	exit(1);



    hDC = GetDC(hWnd);

    hRC = wglCreateContext(hDC);

    wglMakeCurrent(hDC, hRC);



    ShowWindow(hWnd, SW_HIDE);



    /* output header information */

    printf("display: N/A\n");

    printf("server wgl vendor string: N/A\n");

    printf("server wgl version string: N/A\n");

    printf("server wgl extensions (WGL_): N/A\n");

    printf("client wgl version: N/A\n");

    printf("client wgl extensions (WGL_): none\n");

    printf("OpenGL vendor string: %s\n", glGetString(GL_VENDOR));

    printf("OpenGL renderer string: %s\n", glGetString(GL_RENDERER));

    printf("OpenGL version string: %s\n", glGetString(GL_VERSION));

    printf("OpenGL extensions (GL_): \n");



    /* do the magic to separate all extensions with comma's, except

       for the last one that _may_ terminate in a space. */

    i = 0;

    s = (char*)glGetString(GL_EXTENSIONS);

    t[79] = '\0';

    while(*s) {

	t[i++] = *s;

	if(*s == ' ') {

	    if (*(s+1) != '\0') {

		t[i-1] = ',';

		t[i] = ' ';

		p = &t[i++];

	    } else {	       /* zoinks! last one terminated in a space! */

		t[i-1] = '\0';

	    }

	}

	if(i > 80 - 5) {

	    *p = t[i] = '\0';

	    printf("    %s\n", t);

	    p++;

	    i = strlen(p);

	    strcpy(t, p);

	}

	s++;

    }

    t[i] = '\0';

    printf("    %s.\n\n", t);



    /* enumerate all the formats */

    VisualInfo(hDC, verbose);



    PostQuitMessage(0);

    while(GetMessage(&msg, hWnd, 0, 0)) {

	TranslateMessage(&msg);

	DispatchMessage(&msg);

    }



    wglMakeCurrent(NULL, NULL);

    ReleaseDC(hWnd,hDC);

    wglDeleteContext(hRC);

    DestroyWindow(hWnd);



    return msg.wParam;

}

