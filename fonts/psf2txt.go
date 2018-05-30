package main

import (
	"flag"
	"fmt"
	"io"
	"log"
	"os"
)

var psf2txt = flag.Bool("t", false, "Convert psf to txt")
var txt2psf = flag.Bool("p", false, "Convert txt to psf")
var inFileName = flag.String("i", "-", "Input file name. '-' for stdin")
var outFileName = flag.String("o", "-", "Output file name. '-' for stdout")

var inFile *os.File
var outFile *os.File

// Glyph data and unicode mapping
type Glyph struct {
	Data    []byte
	Unicode []rune
}

// Psf2 header and glyphs
type Psf2 struct {
	// Header
	Magic      uint32
	Version    uint32
	HeaderSize uint32 // offset of bitmaps in file
	Flags      uint32 // & 0x01 Has unicode table
	Length     uint32 // number of glyphs
	CharSize   uint32 // number of bytes for each character
	Height     uint32 // Vertical max dimensions of glyphs
	Width      uint32 // Horisontal max dimensions of glyphs
	Glyphs     []Glyph
}

// CheckMagic
func (psf Psf2) CheckMagic() error {
	if psf.Magic&0xFFFF == 0x0436 {
		return fmt.Errorf("psf1 not supported")
	}
	if psf.Magic != 0x864AB572 {
		return fmt.Errorf("it is not a psf2 file")
	}
	return nil
}

func readUint32(r io.Reader) (uint32, error) {
	buf := make([]byte, 4)
	len, err := r.Read(buf)
	if err != nil {
		return 0, err
	}
	if len < 4 {
		return 0, fmt.Errorf("Unexpected end")
	}
	return uint32(buf[0]) |
		uint32(buf[1])<<8 |
		uint32(buf[2])<<16 |
		uint32(buf[3])<<24, nil
}

// ReadHeader
func (psf *Psf2) ReadHeader(r io.Reader) error {
	var err error
	psf.Magic, err = readUint32(r)
	if err != nil {
		return err
	}
	if err := psf.CheckMagic(); err != nil {
		return err
	}

	psf.Version, err = readUint32(r)
	if err != nil {
		return err
	}
	psf.HeaderSize, err = readUint32(r)
	if err != nil {
		return err
	}
	psf.Flags, err = readUint32(r)
	if err != nil {
		return err
	}
	psf.Length, err = readUint32(r)
	if err != nil {
		return err
	}
	psf.CharSize, err = readUint32(r)
	if err != nil {
		return err
	}
	psf.Height, err = readUint32(r)
	if err != nil {
		return err
	}
	psf.Width, err = readUint32(r)
	if err != nil {
		return err
	}
	return nil
}

// ReadPsf read binary file
func (psf *Psf2) ReadPsf(r io.Reader) error {
	var err error
	err = psf.ReadHeader(r)
	if err != nil {
		return err
	}

	// Read glyphs
	psf.Glyphs = make([]Glyph, psf.Length)
	for i := uint32(0); i < psf.Length; i++ {
		psf.Glyphs[i].Data = make([]byte, psf.CharSize)

		len, err := r.Read(psf.Glyphs[i].Data)
		if err != nil {
			return err
		}
		if len < int(psf.CharSize) {
			return fmt.Errorf("Unexpected end")
		}

	}

	return nil
}

func (psf Psf2) WriteTxt(w io.Writer) error {
	fmt.Fprintln(w, "VERSION", psf.Version)
	fmt.Fprintln(w, "HEADERSIZE", psf.HeaderSize)
	fmt.Fprintln(w, "FLAGS", psf.Flags)
	fmt.Fprintln(w, "LENGTH", psf.Length)
	fmt.Fprintln(w, "CHARSIZE", psf.CharSize)
	fmt.Fprintln(w, "HEIGHT", psf.Height)
	fmt.Fprintln(w, "WIDTH", psf.Width)

	for _, glyph := range psf.Glyphs {
		fmt.Fprintln(w, "CHAR")
		gidx := 0
		for row := uint32(0); row < psf.Height; row++ {
			bidx := 0
			b := glyph.Data[gidx]
			gidx++
			for col := uint32(0); col < psf.Width; col++ {
				if bidx >= 8 {
					bidx = 0
					b = glyph.Data[gidx]
					gidx++
				}
				if b&0x80 == 0x80 {
					fmt.Fprint(w, "#")
				} else {
					fmt.Fprint(w, "-")
				}
				b <<= 1
				bidx++
			}
			fmt.Fprintln(w)
		}
	}
	return nil
}

func main() {
	var err error

	log.SetOutput(os.Stderr)
	log.SetFlags(log.Lmicroseconds)

	flag.Parse()

	if *inFileName != "-" {
		inFile, err = os.Open(*inFileName)
		if err != nil {
			log.Fatal(err)
		}
		defer inFile.Close()
	} else {
		inFile = os.Stdin
	}

	if *outFileName != "-" {
		outFile, err = os.Open(*outFileName)
		if err != nil {
			log.Fatal(err)
		}
		defer outFile.Close()
	} else {
		outFile = os.Stdout
	}

	var psf Psf2

	if err := psf.ReadPsf(inFile); err != nil {
		log.Fatal(err)
	}

	psf.WriteTxt(outFile)

}
