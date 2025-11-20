#ifndef INCLUDED_BINARY_STREAM_READERS_H
#define INCLUDED_BINARY_STREAM_READERS_H

//*****************************************************************************
// Class BinaryReader
//*****************************************************************************

class BinaryReader
{
  public:
    bool m_eof;
    char m_filename[256];

    BinaryReader();
    virtual ~BinaryReader();

    virtual bool IsOpen() = 0;
    virtual char* GetFileType() = 0;

    virtual signed char ReadS8() = 0;
    virtual short ReadS16() = 0;
    virtual int ReadS32() = 0;
    virtual unsigned char ReadU8() = 0;
    //	virtual unsigned short	ReadU16	() = 0;
    //	virtual unsigned int	ReadU32	() = 0;

    virtual unsigned int ReadBytes(unsigned int _count, unsigned char* _buffer) = 0;

    virtual int Seek(int _offset, int _origin) = 0;
    virtual int Tell() = 0;

    int GetSize();
};

//*****************************************************************************
// Class BinaryFileReader
//*****************************************************************************

class BinaryFileReader : public BinaryReader
{
  protected:
    FILE* m_file;

  public:
    BinaryFileReader(const char* _filename);
    ~BinaryFileReader() override;

    bool IsOpen() override;
    char* GetFileType() override;

    signed char ReadS8() override;
    short ReadS16() override;
    int ReadS32() override;
    unsigned char ReadU8() override;
    //	unsigned short	ReadU16		();
    //	unsigned int	ReadU32		();

    unsigned int ReadBytes(unsigned int _count, unsigned char* _buffer) override;

    int Seek(int _offset, int _origin) override;
    int Tell() override;
};

//*****************************************************************************
// Class BinaryDataReader
//*****************************************************************************

class BinaryDataReader : public BinaryReader
{
  protected:
    unsigned int m_offset;

  public:
    const unsigned char* m_data;
    unsigned int m_dataSize;

    BinaryDataReader(const unsigned char* _data, unsigned int _dataSize, const char* _filename);
    ~BinaryDataReader() override;

    bool IsOpen() override;
    char* GetFileType() override;

    signed char ReadS8() override;
    short ReadS16() override;
    int ReadS32() override;
    unsigned char ReadU8() override;
    //	unsigned short	ReadU16		();
    //	unsigned int	ReadU32		();

    unsigned int ReadBytes(unsigned int _count, unsigned char* _buffer) override;

    int Seek(int _offset, int _origin) override;
    int Tell() override;
};

#endif
