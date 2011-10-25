
#ifndef BSTDFILE_H
#define BSTDFILE_H

/****************************************************************************
 * Datatypes definitions													*
 ****************************************************************************/
typedef struct bstdfile bstdfile_t;

/****************************************************************************
 * Prototypes																*
 ****************************************************************************/
extern bstdfile_t	*NewBstdFile(FILE *fp);
extern int			BstdFileDestroy(bstdfile_t *BstdFile);
extern int			BstdFileEofP(const bstdfile_t *BstdFile);
extern int			BstdFileErrorP(const bstdfile_t *BstdFile);
extern size_t		BstdRead(void *UserBuffer, size_t ElementSize, size_t ElementsCount, bstdfile_t *BstdFile);
void			BstdSeek(bstdfile_t *BstdFile, long pos, int whence);
long			BstdTell(bstdfile_t *BstdFile);

#endif /* BSTDFILE_H */

/*  LocalWords:  HTAB bstdfile fread Datatypes
 */
/*
 * Local Variables:
 * tab-width: 4
 * End:
 */

/****************************************************************************
 * End of file bstdfile.h													*
 ****************************************************************************/
